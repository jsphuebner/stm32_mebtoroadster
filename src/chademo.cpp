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

void ChaDeMo::UpdateParams(MebBms& mebBms)
{
   const float soc = MIN(100.0f, MAX(0.0f, mebBms.EstimateSocFromVoltage()));
   const float batteryMaxCurrent = mebBms.GetMaximumChargeCurrent(4200.0f);
   const float userLimitCurrent = Param::GetFloat(Param::cdmcurlim);
   const float chargerLimitCurrent = chargerMaxCurrent;
   const float chargeRequest = MIN(255.0f, MAX(0.0f, MIN(chargerLimitCurrent, MIN(batteryMaxCurrent, userLimitCurrent))));
   const float targetVoltage = MebBms::NumCells * 4.2f;

   Param::SetFloat(Param::cdm_bat_vtg, mebBms.GetTotalVoltage());
   Param::SetFloat(Param::cdm_target_vtg, targetVoltage);
   Param::SetFloat(Param::cdm_soc, soc);
   Param::SetInt(Param::cdm_enabled, soc < 100.0f);
   Param::SetInt(Param::cdm_cur_req, (int)chargeRequest);

   Param::SetInt(Param::cdm_chg_max_cur, chargerMaxCurrent);
   Param::SetInt(Param::cdm_chg_cur, chargerOutputCurrent);
   Param::SetInt(Param::cdm_chg_vtg, chargerOutputVoltage);
   Param::SetInt(Param::cdm_chg_status, chargerStatus);
}
