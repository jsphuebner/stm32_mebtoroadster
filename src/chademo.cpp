/*
 * This file is part of the stm32_mebtoroadster project.
 *
 * Copyright (C) 2018 Johannes Huebner <dev@johanneshuebner.com>
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
#include "chademo.h"
#include "mebbms.h"
#include "my_math.h"
#include "params.h"

uint8_t ChaDeMo::chargerMaxCurrent;
uint16_t ChaDeMo::chargerOutputVoltage;
uint8_t ChaDeMo::chargerOutputCurrent;
uint8_t ChaDeMo::chargerStatus;
float ChaDeMo::estimatedSoc;
float ChaDeMo::chargeAddedAs;
uint16_t ChaDeMo::noCurrentTicks;
bool ChaDeMo::chargeSessionActive;

ChaDeMo::ChaDeMo(CanHardware* hw)
{
   canHardware = hw;
   canHardware->AddCallback(this);
   HandleClear();
}

void ChaDeMo::HandleClear()
{
   canHardware->RegisterUserMessage(0x108);
   canHardware->RegisterUserMessage(0x109);
   chargerMaxCurrent = 0;
   chargerOutputVoltage = 0;
   chargerOutputCurrent = 0;
   chargerStatus = 0;
   estimatedSoc = 0;
   chargeAddedAs = 0;
   noCurrentTicks = 0;
   chargeSessionActive = false;
   ResetParams();
}

void ChaDeMo::HandleRx(uint32_t canId, uint32_t data[2], uint8_t)
{
   if (canId == 0x108)
   {
      chargerMaxCurrent = data[0] >> 24;
   }
   else if (canId == 0x109)
   {
      chargerOutputVoltage = data[0] >> 8;
      chargerOutputCurrent = data[0] >> 24;
      chargerStatus = (data[1] >> 8) & 0x3F;
   }
}

void ChaDeMo::SetupCanMap(CanMap* canMap)
{
   // 0x100: battery capability message
   canMap->AddSend(Param::cdm_bat_vtg,    0x100, 0,  16, 1.0f);
   canMap->AddSend(Param::cdm_target_vtg, 0x100, 32, 16, 1.0f, 40);

   // 0x101: fixed compatibility message data (0x00FEFF00)
   canMap->AddSend(Param::cdm_bat_vtg,    0x101, 8,   8, 0.0f, -1);
   canMap->AddSend(Param::cdm_bat_vtg,    0x101, 16,  8, 0.0f, -2);

   // 0x102: main vehicle status message
   canMap->AddSend(Param::cdm_bat_vtg,    0x102, 0,   8, 0.0f, 1);
   canMap->AddSend(Param::cdm_target_vtg, 0x102, 8,  16, 1.0f);
   canMap->AddSend(Param::cdm_cur_req,    0x102, 24,  8, 1.0f);
   canMap->AddSend(Param::cdm_enabled,    0x102, 40,  1, 1.0f);
   canMap->AddSend(Param::cdm_soc,        0x102, 48,  8, 2.0f);
}

/**
 * Called once at startup after parm_load(). Checks whether the CHaDeMo send
 * mapping for cdm_target_vtg is still present on CAN ID 0x102. If the user has
 * cleared the CAN map the mapping will be absent, and the full default set is
 * re-established by calling SetupCanMap().
 */
void ChaDeMo::CheckAndRestoreCanMap(CanMap* canMap)
{
   uint32_t canId;
   uint8_t start;
   int8_t length;
   float gain;
   int8_t offset;
   bool rx;

   if (!canMap->FindMap(Param::cdm_target_vtg, canId, start, length, gain, offset, rx)
       || canId != 0x102 || rx)
   {
      SetupCanMap(canMap);
   }
}

void ChaDeMo::ResetParams()
{
   Param::SetFloat(Param::cdm_bat_vtg, 0);
   Param::SetFloat(Param::cdm_target_vtg, 0);
   Param::SetFloat(Param::cdm_soc, 0);
   Param::SetFloat(Param::cdm_charge_added, 0);
   Param::SetInt(Param::cdm_enabled, 0);
   Param::SetInt(Param::cdm_cur_req, 0);
   Param::SetInt(Param::cdm_chg_max_cur, 0);
   Param::SetInt(Param::cdm_chg_cur, 0);
   Param::SetInt(Param::cdm_chg_vtg, 0);
   Param::SetInt(Param::cdm_chg_status, 0);
}

void ChaDeMo::UpdateParams(MebBms& mebBms)
{
   if (chargerStatus == 0)
   {
      chargeAddedAs = 0;
      noCurrentTicks = 0;
      chargeSessionActive = false;
      ResetParams();
      return;
   }

   if (!chargeSessionActive)
   {
      estimatedSoc = MIN(100.0f, MAX(0.0f, mebBms.EstimateSocFromVoltage()));
      chargeAddedAs = 0;
      noCurrentTicks = 0;
      chargeSessionActive = true;
   }

   if (chargerOutputCurrent > 0)
   {
      chargeAddedAs += chargerOutputCurrent * 0.1f;
      noCurrentTicks = 0;
   }
   else if (noCurrentTicks < UINT16_MAX)
   {
      noCurrentTicks++;
   }

   if (noCurrentTicks >= 1800)
   {
      estimatedSoc = MIN(100.0f, MAX(0.0f, mebBms.EstimateSocFromVoltage()));
      chargeAddedAs = 0;
   }

   const float soc = MIN(100.0f, MAX(0.0f, estimatedSoc + (100.0f * (chargeAddedAs / 3600.0f) / MAX(1.0f, mebBms.GetMaximumAmpHours()))));
   const float cellMaxVoltage = Param::GetFloat(Param::cellmax);
   const float batteryMaxCurrent = mebBms.GetMaximumChargeCurrent(cellMaxVoltage);
   const float userLimitCurrent = Param::GetFloat(Param::cdmcurlim);
   const float chargerLimitCurrent = chargerMaxCurrent;
   const float chargeRequest = MIN(255.0f, MAX(0.0f, MIN(chargerLimitCurrent, MIN(batteryMaxCurrent, userLimitCurrent))));
   const float targetVoltage = MebBms::NumCells * (cellMaxVoltage / 1000.0f);

   Param::SetFloat(Param::cdm_bat_vtg, mebBms.GetTotalVoltage());
   Param::SetFloat(Param::cdm_target_vtg, targetVoltage);
   Param::SetFloat(Param::cdm_soc, soc);
   Param::SetFloat(Param::cdm_charge_added, chargeAddedAs);
   Param::SetInt(Param::cdm_enabled, soc < 100.0f);
   Param::SetInt(Param::cdm_cur_req, (int)chargeRequest);

   Param::SetInt(Param::cdm_chg_max_cur, chargerMaxCurrent);
   Param::SetInt(Param::cdm_chg_cur, chargerOutputCurrent);
   Param::SetInt(Param::cdm_chg_vtg, chargerOutputVoltage);
   Param::SetInt(Param::cdm_chg_status, chargerStatus);
}
