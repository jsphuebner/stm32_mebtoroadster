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

class ChaDeMo
{
   public:
      /** Register all CHaDeMo send mappings in the given CanMap */
      static void SetupCanMap(CanMap* canMap);
      /** Check that CHaDeMo mappings are still present; restore them if they were erased */
      static void CheckAndRestoreCanMap(CanMap* canMap);
};

#endif // CHADEMO_H
