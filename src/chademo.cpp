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
#include "params.h"

/**
 * Register all CHaDeMo transmit mappings in the supplied CanMap.
 *
 * CAN frame layout (all little-endian / Intel byte order):
 *
 *  0x100 – battery capability
 *    bits  0-15  cdm_bat_vtg     actual battery voltage (V)
 *    bits 32-47  cdm_target_vtg  target voltage + 40 (V)
 *    bits 48-55  cdm_capacity    capacity / SoC denominator (default 200)
 *
 *  0x101 – max voltage compatibility
 *    bits  0-15  cdm_target_vtg  maximum acceptable battery voltage (V)
 *
 *  0x102 – main vehicle status
 *    bits  0-7   cdm_version     CHaDeMo protocol version (default 1)
 *    bits  8-23  cdm_target_vtg  target battery voltage (V)
 *    bits 24-31  cdm_cur_req     charge current request (A)
 *    bit  40     cdm_enabled     charge enable flag
 *    bit  41     cdm_parking     parking position flag
 *    bit  42     cdm_fault       fault flag
 *    bit  43     cdm_contactor   contactor-open flag
 *    bits 48-55  cdm_soc         SoC × 2 (0–100 % → 0–200 in frame)
 */
void ChaDeMo::SetupCanMap(CanMap* canMap)
{
   // 0x100: battery capability message
   canMap->AddSend(Param::cdm_bat_vtg,    0x100, 0,  16, 1.0f);
   canMap->AddSend(Param::cdm_target_vtg, 0x100, 32, 16, 1.0f, 40);
   canMap->AddSend(Param::cdm_capacity,   0x100, 48,  8, 1.0f);

   // 0x101: max voltage compatibility / charging-time message
   canMap->AddSend(Param::cdm_target_vtg, 0x101, 0,  16, 1.0f);

   // 0x102: main vehicle status message
   canMap->AddSend(Param::cdm_version,    0x102, 0,   8, 1.0f);
   canMap->AddSend(Param::cdm_target_vtg, 0x102, 8,  16, 1.0f);
   canMap->AddSend(Param::cdm_cur_req,    0x102, 24,  8, 1.0f);
   canMap->AddSend(Param::cdm_enabled,    0x102, 40,  1, 1.0f);
   canMap->AddSend(Param::cdm_parking,    0x102, 41,  1, 1.0f);
   canMap->AddSend(Param::cdm_fault,      0x102, 42,  1, 1.0f);
   canMap->AddSend(Param::cdm_contactor,  0x102, 43,  1, 1.0f);
   canMap->AddSend(Param::cdm_soc,        0x102, 48,  8, 2.0f);
}

/**
 * Called once at startup after parm_load(). Checks whether the CHaDeMo send
 * mapping for cdm_enabled is still present on CAN ID 0x102. If the user has
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

   if (!canMap->FindMap(Param::cdm_enabled, canId, start, length, gain, offset, rx)
       || canId != 0x102 || rx)
   {
      SetupCanMap(canMap);
   }
}
