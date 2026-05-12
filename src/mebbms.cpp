/*
 * This file is part of the stm32_mebtoroadster project.
 *
 * Copyright (C) 2021 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/* This code is based on Tom's VWBms implementation: https://github.com/Tom-evnut/VW-bms */
#include "mebbms.h"
#include "my_math.h"

#define FIRST_VTG_ID    0x1C0

const uint16_t vtgToSoc[] =
{/*2.85V  2.90  2.95  3.00  3.05 3.10  3.15  3.20  3.25  3.30  3.35  3.40  3.45  3.50  3.55  3.60  3.65  3.70  3.75  3.80  3.85  3.90  3.95  4.00  4.05  4.10  4.15  4.20  */
   0,     12,   31,   55,   80,   116,  153,  202,  239,  325,  496,  701,  1231, 1794, 2307, 3368, 4388, 5368, 5949, 6326, 6742, 7232, 7708, 8104, 8575, 8996, 9446, 10000
};

const uint16_t socToSoe[] =
{/*0    5%    10%    15%   20%   25%   30%   35%   40%  45%    50%   55%   60%   65%   70%   75%   80%   85%   90%   95%   100% */
   0,   415,  870,   1334, 1803, 2278, 2758, 3241, 3727, 4216, 4709, 5206, 5707, 6217, 6734, 7259, 7791, 8331, 8879, 9434, 10000
};

static const uint16_t tableMinVtg = 2850;
static const uint16_t tableMaxVtg = 4200;
static const uint8_t socCurveTableItems = sizeof(vtgToSoc) / sizeof(vtgToSoc[0]);
static const uint8_t socCurveGranularity = 50;
static const uint8_t energyCurveTableItems = sizeof(socToSoe) / sizeof(socToSoe[0]);
static const float energyCurveGranularity = 100.0f / (energyCurveTableItems - 1);

MebBms::MebBms(CanHardware* c)
   : canHardware(c), maxCellVoltage(0), minCellVoltage(0), filteredMaxCellVoltage(0), totalVoltage(0), lowTemp(0), highTemp(0),
     maxAh(148), balCounter(0)
{
   for (int i = 0; i < NumCells; i++)
      cellVoltages[i] = 0;

   for (int i = 0; i < (NumCells / CellsPerCmu); i++)
   {
      temps[i] = 0;
      lastReceived[i] = 0;
      balFlags[i] = 0;
      balancerRunning[i] = false;
   }

   cvControllers[0].SetRef(FP_FROMINT(3950));
   cvControllers[1].SetRef(FP_FROMINT(4050));
   cvControllers[2].SetRef(FP_FROMINT(4200));
   SetControllerGains(3, 3);

   canHardware->AddCallback(this);
   HandleClear();
}

void MebBms::SetControllerGains(int kp, int ki)
{
   for (int i = 0; i < 3; i++)
   {
      cvControllers[i].SetGains(kp, ki);
      cvControllers[i].SetCallingFrequency(10);
      cvControllers[i].ResetIntegrator();
   }
}

void MebBms::HandleClear()
{
   canHardware->RegisterUserMessage(0x1C0, 0x7F0);
   canHardware->RegisterUserMessage(0x1D0, 0x7F0);
   canHardware->RegisterUserMessage(0x1A5555F4);
   canHardware->RegisterUserMessage(0x1A5555F5);
   canHardware->RegisterUserMessage(0x1A5555F6);
   canHardware->RegisterUserMessage(0x1A5555F7);
   canHardware->RegisterUserMessage(0x1A5555F8);
   canHardware->RegisterUserMessage(0x1A5555F9);
   canHardware->RegisterUserMessage(0x1A5555FA);
   canHardware->RegisterUserMessage(0x1A5555FB);
}

void MebBms::HandleRx(uint32_t canId, uint32_t data[2], uint8_t)
{
   if (canId >= FIRST_VTG_ID && canId <= 0x1DF)
   {
      if ((canId & 3) != 3)
      {
         int group = ((canId - FIRST_VTG_ID + 1) * 3) / 4;
         int cmu = group / 3;

         if (!balancerRunning[cmu])
         {
            SetCellVoltage(group * 4, ((data[0] >> 12) & 0xFFF) + 1000);
            SetCellVoltage(group * 4 + 1, ((data[0] >> 24) | ((data[1] & 0xF) << 8)) + 1000);
            SetCellVoltage(group * 4 + 2, ((data[1] >> 4) & 0xFFF) + 1000);
            SetCellVoltage(group * 4 + 3, ((data[1] >> 16) & 0xFFF) + 1000);
         }
      }
   }
   else if (canId >= 0x1A5555F4 && canId <= 0x1A5555FB)
   {
      int cmu = (canId & 0xF) - 4;
      temps[cmu] = ((data[1] >> 4) & 0xFF) * 0.5f - 40;
      lastReceived[cmu] = canHardware->GetLastRxTimestamp();

      if (cmu == 0)
      {
         float min = 100.0f, max = -100.0f;

         for (int i = 0; i < 8; i++)
         {
            min = MIN(min, temps[i]);
            max = MAX(max, temps[i]);
         }
         lowTemp = min;
         highTemp = max;
      }
   }
}

float MebBms::GetMaximumChargeCurrent(float cellmax)
{
   const float lowTempDerate = LowTempDerating();
   const float highTempDerate = HighTempDerating(51);
   const float cc1Current = 275.0f * lowTempDerate;
   const float cc2Current = 170.0f * lowTempDerate;
   const float cc3Current = 114.0f * lowTempDerate;
   int32_t result;

   cvControllers[0].SetMinMaxY(0, cc1Current);
   cvControllers[1].SetMinMaxY(0, cc2Current);
   cvControllers[2].SetMinMaxY(0, cc3Current);
   cvControllers[2].SetRef(FP_FROMFLT(cellmax));

   int32_t cv1Result = cvControllers[0].Run(FP_FROMFLT(filteredMaxCellVoltage));
   int32_t cv2Result = cvControllers[1].Run(FP_FROMFLT(filteredMaxCellVoltage));
   int32_t cv3Result = cvControllers[2].Run(FP_FROMFLT(filteredMaxCellVoltage));

   result = MAX(cv1Result, MAX(cv2Result, cv3Result));
   result *= highTempDerate;

   return result;
}

float MebBms::GetMaximumDischargeCurrent(float cellVoltageCutoff)
{
   const float highTempDerate = HighTempDerating(53);
   const float maxDischargeCurrent = 500;
   float result = (minCellVoltage - cellVoltageCutoff) * 5;
   result = MIN(maxDischargeCurrent, result);
   result = MAX(result, 0);
   result *= highTempDerate;

   return result;
}

float MebBms::EstimateSocFromVoltage()
{
   int lookupVoltage = MIN(tableMaxVtg, minCellVoltage) - tableMinVtg;
   lookupVoltage = MAX(socCurveGranularity, lookupVoltage);
   int socIndex = lookupVoltage / socCurveGranularity;
   float socFraction = (lookupVoltage - (socIndex * socCurveGranularity)) / (float)socCurveGranularity;
   float diff = vtgToSoc[socIndex + 1] - vtgToSoc[socIndex];
   float soc = vtgToSoc[socIndex] + diff * socFraction;

   return soc / 100;
}

float MebBms::GetRemainingEnergy(float soc)
{
   const float nominalVoltage = 3.67f;
   int socIndex = soc / energyCurveGranularity;
   float socFraction = (soc - (socIndex * energyCurveGranularity)) / (float)energyCurveGranularity;
   socIndex = MIN(socIndex, energyCurveTableItems - 1);
   socIndex = MAX(socIndex, 0);
   float energyAtSoc;

   if (socIndex < (energyCurveTableItems - 1))
   {
      float diff = socToSoe[socIndex + 1] - socToSoe[socIndex];
      energyAtSoc = (float)socToSoe[socIndex] + diff * socFraction;
   }
   else
   {
      energyAtSoc = socToSoe[energyCurveTableItems - 1];
   }

   return energyAtSoc * nominalVoltage * NumCells * GetMaximumAmpHours() / 10000;
}

void MebBms::Balance(bool enable, int& start)
{
   const uint16_t balHyst = 4;
   const uint16_t balMin = 3730;
   uint8_t balCmds[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFE, 0xFE, 0xFE, 0xFE };
   bool balance = enable && balCounter < 3;
   bool balancing = false;

   if (start == 0)
   {
      balCounter++;
      balCounter &= 0x3;
   }

   for (int i = start; i < NumCells; i++)
   {
      const int group = i / CellsPerCmu;
      const int cell = i % CellsPerCmu;
      const bool balFlag = (cellVoltages[i] > (minCellVoltage + balHyst)) && (cellVoltages[i] > balMin);

      balCmds[cell] = balFlag && balance ? 0x8 : 0x0;
      balancing |= balFlag && balance;
      balancerRunning[group] = balancing;

      if (cell == 0) balFlags[group] = 0;
      balFlags[group] |= balFlag << cell;

      if (cell == 7)
      {
         uint32_t canId = (group < 5 ? 0x1A555412 : 0x1A5554A1) + group * 2;
         canHardware->Send(canId, (uint32_t*)balCmds);
      }
      else if (cell == (CellsPerCmu - 1))
      {
         uint32_t canId = (group < 5 ? 0x1A555413 : 0x1A5554A2) + group * 2;
         canHardware->Send(canId, (uint32_t*)&balCmds[8]);
         start = i + 1;
         start = start == NumCells ? 0 : start;
         break;
      }
   }
}

bool MebBms::Alive(uint32_t time)
{
   uint32_t lastRecv = time;

   for (int i = 0; i < (NumCells / CellsPerCmu); i++)
   {
      lastRecv = MIN(lastReceived[i], lastRecv);
   }
   return (time - lastRecv) < 100;
}

void MebBms::Accumulate()
{
   float min = 4500, max = 0, sum = 0;

   for (int i = 0; i < NumCells; i++)
   {
      float voltage = cellVoltages[i];

      if (voltage > 1000)
      {
         sum += voltage;
         min = MIN(min, voltage);
         max = MAX(max, voltage);
      }
   }

   minCellVoltage = min;
   maxCellVoltage = max;
   totalVoltage = sum;

   if (filteredMaxCellVoltage == 0) filteredMaxCellVoltage = maxCellVoltage;
   else filteredMaxCellVoltage = IIRFILTERF(filteredMaxCellVoltage, maxCellVoltage, 4);
}

float MebBms::LowTempDerating()
{
   const float drt1Temp = 25.0f;
   const float drt2Temp = 0;
   const float drt3Temp = -20.0f;
   const float factorAtDrt2 = 0.3f;
   float factor;

   if (lowTemp > drt1Temp)
      factor = 1;
   else if (lowTemp > drt2Temp)
      factor = factorAtDrt2 + (1 - factorAtDrt2) * (lowTemp - drt2Temp) / (drt1Temp - drt2Temp);
   else if (lowTemp > drt3Temp)
      factor = factorAtDrt2 * (lowTemp - drt3Temp) / (drt2Temp - drt3Temp);
   else
      factor = 0;

   return factor;
}

float MebBms::HighTempDerating(float maxTemp)
{
   float factor = (maxTemp - highTemp) * 0.15f;
   factor = MIN(1, factor);
   factor = MAX(0, factor);

   return factor;
}

void MebBms::SetCellVoltage(int idx, int vtg)
{
   if (vtg < 1000 || vtg > 4400) return;
   if (cellVoltages[idx] == 0)
      cellVoltages[idx] = vtg;
   else
      cellVoltages[idx] = IIRFILTERF(cellVoltages[idx], (float)vtg, 2);
}
