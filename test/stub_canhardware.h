/*
 * This file is part of the stm32_mebtoroadster project.
 *
 * Copyright (C) 2025 Johannes Huebner <dev@johanneshuebner.com>
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
#ifndef MULTI_CANHARDWARE_H
#define MULTI_CANHARDWARE_H

#include "canhardware.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

/**
 * A CanHardware stub that accumulates all outgoing frames in a vector and
 * allows the test to inspect them after calling Update().
 *
 * It derives from CanHardware so that AddCallback() / RegisterUserMessage() /
 * HandleRx() from the real canhardware.cpp implementation are used.  The
 * protected lastRxTimestamp member is exposed via SetLastRxTimestamp() so that
 * tests can make MebBms::Alive() return a known value.
 */
class MultiCanStub : public CanHardware
{
public:
   struct Frame
   {
      uint32_t                canId;
      std::array<uint8_t, 8> data;
      uint8_t                 len;
   };

   MultiCanStub() { Clear(); }

   void SetBaudrate(enum baudrates) override {}
   void ConfigureFilters() override {}

   void Send(uint32_t canId, uint32_t data[2], uint8_t len, bool /*forceExt*/) override
   {
      Frame f;
      f.canId = canId;
      f.len   = len;
      std::memcpy(f.data.data(), data, 8);
      sentFrames.push_back(f);
   }

   /** Allow tests to set the RTC timestamp reported to MebBms. */
   void SetLastRxTimestamp(uint32_t ts) { lastRxTimestamp = ts; }

   void Clear() { sentFrames.clear(); }

   /** Find the first frame sent on canId whose first data byte equals mux. */
   const Frame* FindFrame(uint32_t canId, uint8_t mux) const
   {
      for (const auto& f : sentFrames)
         if (f.canId == canId && f.data[0] == mux)
            return &f;
      return nullptr;
   }

   /** Count frames sent on canId whose first data byte equals mux. */
   int CountFrames(uint32_t canId, uint8_t mux) const
   {
      int n = 0;
      for (const auto& f : sentFrames)
         if (f.canId == canId && f.data[0] == mux)
            n++;
      return n;
   }

   std::vector<Frame> sentFrames;
};

#endif // MULTI_CANHARDWARE_H
