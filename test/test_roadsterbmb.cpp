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

#include "roadsterbmb.h"
#include "mebbms.h"
#include "params.h"
#include "stub_canhardware.h"
#include "test.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>

// ---------------------------------------------------------------------------
// Utility: pretty-print a CAN frame
// ---------------------------------------------------------------------------
static std::ostream& operator<<(std::ostream& o, const MultiCanStub::Frame& f)
{
   o << "id=0x" << std::hex << std::setw(3) << std::setfill('0') << f.canId << " [";
   for (int i = 0; i < f.len; i++)
      o << "0x" << std::setw(2) << std::setfill('0') << (int)f.data[i] << " ";
   o << "]";
   return o;
}

// ---------------------------------------------------------------------------
// CAN ID constants (mirror those in roadsterbmb.cpp)
// ---------------------------------------------------------------------------
static const uint32_t NodeBroadcastId      = 0x006;
static const uint32_t NodeReplyBaseId      = 0x30A;
static const uint32_t CellAvgReplyBaseId   = 0x308; // base for cell avg msgs 0 and 1
static const uint32_t NodeIdStride         = 8;

// ---------------------------------------------------------------------------
// Helpers to build raw CAN data arrays
// ---------------------------------------------------------------------------
static uint32_t gData[2]; // reusable scratch for HandleRx calls

static void SendFrame(MultiCanStub& stub, uint32_t canId,
                      uint8_t b0, uint8_t b1 = 0, uint8_t b2 = 0, uint8_t b3 = 0,
                      uint8_t b4 = 0, uint8_t b5 = 0, uint8_t b6 = 0, uint8_t b7 = 0,
                      uint8_t dlc = 8)
{
   uint8_t buf[8] = { b0, b1, b2, b3, b4, b5, b6, b7 };
   std::memcpy(gData, buf, 8);
   stub.HandleRx(canId, gData, dlc);
}

// ---------------------------------------------------------------------------
// Populate MebBms with uniform cell voltages so RoadsterBmb::Update() sees
// valid data from all 96 cells.
//
// MEB voltage CAN format (from mebbms.cpp):
//   cell0 = ((data[0] >> 12) & 0xFFF) + 1000
//   cell1 = ((data[0] >> 24) | ((data[1] & 0xF) << 8)) + 1000
//   cell2 = ((data[1] >>  4) & 0xFFF) + 1000
//   cell3 = ((data[1] >> 16) & 0xFFF) + 1000
// ---------------------------------------------------------------------------
static void FillMebVoltages(MultiCanStub& stub, uint32_t targetMv)
{
   const uint32_t v = targetMv - 1000u; // v in [0, 0xFFF]
   const uint32_t d0 = (v << 12) | ((v & 0xFF) << 24);
   const uint32_t d1 = (v >> 8) | (v << 4) | (v << 16);

   // CAN IDs 0x1C0 to 0x1DF, skipping those where (id & 3) == 3.
   for (uint32_t id = 0x1C0; id <= 0x1DF; id++)
   {
      if ((id & 3) == 3) continue;
      gData[0] = d0;
      gData[1] = d1;
      stub.HandleRx(id, gData, 8);
   }
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class RoadsterBmbTest : public UnitTest
{
public:
   explicit RoadsterBmbTest(const std::list<VoidFunction>* cases) : UnitTest(cases) {}
   virtual void TestCaseSetup();
};

std::unique_ptr<MultiCanStub> canStub;
std::unique_ptr<MebBms>       mebBms;
std::unique_ptr<RoadsterBmb>  roadster;

// Needed by params.cpp
void Param::Change(Param::PARAM_NUM) {}

void RoadsterBmbTest::TestCaseSetup()
{
   canStub  = std::make_unique<MultiCanStub>();
   mebBms   = std::make_unique<MebBms>(canStub.get());
   roadster = std::make_unique<RoadsterBmb>(canStub.get());
   Param::LoadDefaults();
}

// ---------------------------------------------------------------------------
// Assert helpers
// ---------------------------------------------------------------------------
static bool FramePresent(uint32_t canId, uint8_t b0)
{
   const MultiCanStub::Frame* f = canStub->FindFrame(canId, b0);
   if (!f)
   {
      std::cout << "  Frame with id=0x" << std::hex << canId
                << " b0=0x" << (int)b0 << " not found\n";
   }
   return f != nullptr;
}

static bool FrameIs(uint32_t canId, const std::array<uint8_t, 8>& expected, uint8_t dlc)
{
   const MultiCanStub::Frame* f = canStub->FindFrame(canId, expected[0]);
   if (!f)
   {
      std::cout << "  Frame id=0x" << std::hex << canId
                << " b0=0x" << (int)expected[0] << " not found\n";
      return false;
   }
   if (f->len != dlc)
   {
      std::cout << "  Frame dlc mismatch: expected " << (int)dlc
                << " got " << (int)f->len << "\n";
      return false;
   }
   for (int i = 0; i < dlc; i++)
   {
      if (f->data[i] != expected[i])
      {
         std::cout << "  Frame data mismatch at byte " << i
                   << ": expected 0x" << std::hex << (int)expected[i]
                   << " got 0x" << (int)f->data[i] << "\n";
         return false;
      }
   }
   return true;
}

// ---------------------------------------------------------------------------
// Test: 0x006 0x1F 0x00 triggers BroadcastInfoReply on all 11 sheet channels
// ---------------------------------------------------------------------------
static void test_broadcast_info_reply()
{
   roadster->Update(*mebBms, 2);

   SendFrame(*canStub, NodeBroadcastId, 0x1F, 0x00, 0, 0, 0, 0, 0, 0, 2);

   canStub->Clear();
   roadster->Update(*mebBms, 3);

   bool ok = true;
   for (int sheet = 0; sheet < 11; sheet++)
   {
      uint32_t replyId = NodeReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride;
      if (!FramePresent(replyId, 0x1F))
      {
         std::cout << "  Missing BroadcastInfo reply for sheet " << sheet << "\n";
         ok = false;
      }
   }
   ASSERT(ok);
}

// ---------------------------------------------------------------------------
// Test: 0x006 0x24 capability request triggers BroadcastCapabilityReply
// ---------------------------------------------------------------------------
static void test_broadcast_capability_reply()
{
   roadster->Update(*mebBms, 2);

   // 24 00 00 01 F0 00 ...
   SendFrame(*canStub, NodeBroadcastId, 0x24, 0x00, 0x00, 0x01, 0xF0, 0x00, 0x00, 0x00, 6);

   canStub->Clear();
   roadster->Update(*mebBms, 3);

   ASSERT(FramePresent(NodeReplyBaseId, 0x24));
}

// ---------------------------------------------------------------------------
// Test: directed 0x23 0x00 to sheet 0 triggers presence reply
// ---------------------------------------------------------------------------
static void test_directed_presence_reply()
{
   roadster->Update(*mebBms, 2);

   SendFrame(*canStub, 0x00A, 0x23, 0x00, 0, 0, 0, 0, 0, 0, 2);

   canStub->Clear();
   roadster->Update(*mebBms, 3);

   ASSERT(FrameIs(NodeReplyBaseId, { 0x23, 0x00, 0xE6, 0x00, 0x00, 0x00, 0x00, 0x00 }, 6));
}

// ---------------------------------------------------------------------------
// Test: 0x006 0x25 request generates 0x20 cell-voltage replies on all sheets
// ---------------------------------------------------------------------------
static void test_cell_avg_reply_on_0x25()
{
   // Fill MEB with 3000 mV cells
   FillMebVoltages(*canStub, 3000);

   roadster->Update(*mebBms, 2); // ensure MEB data is alive

   // Send the 0x25 broadcast request
   SendFrame(*canStub, NodeBroadcastId, 0x25, 0x00, 0x02, 0x01, 0, 0, 0, 0, 4);

   // The implementation sends 3 sheets per Update() call; 4 calls cover all 11 sheets.
   canStub->Clear();
   for (int i = 0; i < 4; i++)
      roadster->Update(*mebBms, 50 + static_cast<uint32_t>(i));

   // Each of the 11 sheets should reply with 3 messages (indices 0, 1, 2)
   // msgIdx 0,1 → CellAvgReplyBaseId + sheet*8; msgIdx 2 → NodeReplyBaseId + sheet*8
   bool ok = true;
   for (int sheet = 0; sheet < 11; sheet++)
   {
      for (int idx = 0; idx < 3; idx++)
      {
         const uint32_t replyId = (idx < 2)
            ? (CellAvgReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride)
            : (NodeReplyBaseId    + static_cast<uint32_t>(sheet) * NodeIdStride);
         bool found = false;
         for (const auto& f : canStub->sentFrames)
         {
            if (f.canId == replyId && f.data[0] == 0x20 && f.data[1] == (uint8_t)idx)
            {
               found = true;
               break;
            }
         }
         if (!found)
         {
            std::cout << "  Missing 0x20 msg idx=" << idx
                      << " for sheet " << sheet
                      << " (replyId=0x" << std::hex << replyId << ")\n";
            ok = false;
         }
      }
   }
   ASSERT(ok);
}

// ---------------------------------------------------------------------------
// Test: 0x25 reply voltage values are correct
//
// For 3000 mV:  RawVoltage = round(3000 * 8.192) = 24576 = 0x6000
//               little-endian bytes: 0x00, 0x60
// ---------------------------------------------------------------------------
static void test_cell_avg_reply_voltage_values()
{
   FillMebVoltages(*canStub, 3000);
   roadster->Update(*mebBms, 2);

   SendFrame(*canStub, NodeBroadcastId, 0x25, 0x00, 0x02, 0x01, 0, 0, 0, 0, 4);

   canStub->Clear();
   roadster->Update(*mebBms, 50);

   // Find the first 0x20 message (idx=0) on sheet 0's reply channel
   // msgIdx 0 uses CellAvgReplyBaseId (= 0x308) for sheet 0
   const uint32_t replyId = CellAvgReplyBaseId; // sheet 0, msgIdx 0 and 1
   bool found = false;
   bool voltageOk = true;

   for (const auto& f : canStub->sentFrames)
   {
      if (f.canId == replyId && f.data[0] == 0x20 && f.data[1] == 0)
      {
         found = true;
         // bytes 2-7: three 16-bit LE voltages, all should be 0x6000
         for (int i = 0; i < 3; i++)
         {
            uint16_t raw = static_cast<uint16_t>(f.data[2 + i * 2]) |
                           (static_cast<uint16_t>(f.data[3 + i * 2]) << 8);
            if (raw != 0x6000)
            {
               std::cout << "  Voltage mismatch at cell " << i
                         << " in msg 0: got 0x" << std::hex << raw
                         << " expected 0x6000\n";
               voltageOk = false;
            }
         }
         break;
      }
   }

   ASSERT(found);
   ASSERT(voltageOk);
}

// ---------------------------------------------------------------------------
// Test: 0x25 reply is suppressed when MebBms is not alive (time too large)
// ---------------------------------------------------------------------------
static void test_cell_avg_reply_suppressed_when_not_alive()
{
   // Do NOT fill MEB voltages; all lastReceived[] stay 0.
   // With time=200, (200 - 0) = 200 >= 100 → Alive() returns false.

   // Send startup
   roadster->Update(*mebBms, 0);

   SendFrame(*canStub, NodeBroadcastId, 0x25, 0x00, 0x02, 0x01, 0, 0, 0, 0, 4);

   canStub->Clear();
   roadster->Update(*mebBms, 200);

   // No 0x20 frames should be sent
   bool found = false;
   for (const auto& f : canStub->sentFrames)
   {
      if (f.data[0] == 0x20)
      {
         found = true;
         break;
      }
   }
   ASSERT(!found);
}

// ---------------------------------------------------------------------------
// Test: replay of fahrbereit-log sequence – 0x25 triggers 0x20 voltage replies
// ---------------------------------------------------------------------------
static void test_fahrbereit_log_replay_cell_avg()
{
   // The fahrbereit log shows 0x006 0x25 0x00 0x02 0x01 requests.
   // Fill MEB, send the request, verify replies exist.

   FillMebVoltages(*canStub, 4020);
   roadster->Update(*mebBms, 1);

   // Replicate: 00000006,4,25,00,02,01
   SendFrame(*canStub, NodeBroadcastId, 0x25, 0x00, 0x02, 0x01, 0, 0, 0, 0, 4);

   canStub->Clear();
   roadster->Update(*mebBms, 50);

   // Verify at least sheet 0 replied with a 0x20 message (msgIdx 0 → CellAvgReplyBaseId)
   ASSERT(FramePresent(CellAvgReplyBaseId, 0x20));
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------
REGISTER_TEST(RoadsterBmbTest,
   test_broadcast_info_reply,
   test_broadcast_capability_reply,
   test_directed_presence_reply,
   test_cell_avg_reply_on_0x25,
   test_cell_avg_reply_voltage_values,
   test_cell_avg_reply_suppressed_when_not_alive,
   test_fahrbereit_log_replay_cell_avg
);
