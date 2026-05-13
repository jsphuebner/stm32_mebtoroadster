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
#ifndef CHADEMO_H
#define CHADEMO_H
#include "canmap.h"
#include "canhardware.h"

class MebBms;

class ChaDeMo : public CanCallback
{
   public:
      explicit ChaDeMo(CanHardware* canHardware);
      void HandleRx(uint32_t canId, uint32_t data[2], uint8_t dlc) override;
      void HandleClear() override;
      /** Register all CHaDeMo send mappings in the given CanMap */
      static void SetupCanMap(CanMap* canMap);
      /** Check that CHaDeMo mappings are still present; restore them if they were erased */
      static void CheckAndRestoreCanMap(CanMap* canMap);
      /** Fill runtime CHaDeMo values from BMS, SoC source and charger telemetry */
      static void UpdateParams(MebBms& mebBms, float soc);
      static uint8_t GetChargerMaxCurrent() { return chargerMaxCurrent; }

   private:
      CanHardware* canHardware;
      static uint8_t chargerMaxCurrent;
      static uint16_t chargerOutputVoltage;
      static uint8_t chargerOutputCurrent;
      static uint8_t chargerStatus;
};

#endif // CHADEMO_H
