#include <cmath>
#include <limits.h>
#include "roadsterbmb.h"
#include "my_math.h"

#define SHEET_PARAMS(sheet) \
   { \
      Param::bmb##sheet##_bal_min_v, \
      Param::bmb##sheet##_bal_min_brick, \
      Param::bmb##sheet##_bal_max_v, \
      Param::bmb##sheet##_bal_max_brick, \
      Param::bmb##sheet##_v_min, \
      Param::bmb##sheet##_v_max, \
      Param::bmb##sheet##_v_sum_avg, \
      Param::bmb##sheet##_v_min_brick, \
      Param::bmb##sheet##_v_max_brick, \
      Param::bmb##sheet##_t_min, \
      Param::bmb##sheet##_t_max, \
      Param::bmb##sheet##_t_avg, \
      Param::bmb##sheet##_t_min_therm, \
      Param::bmb##sheet##_t_max_therm, \
      Param::bmb##sheet##_bleed, \
      Param::bmb##sheet##_daisy_rx, \
      Param::bmb##sheet##_over_v, \
      Param::bmb##sheet##_cell_reversal, \
      Param::bmb##sheet##_can_pwr_ok, \
      Param::bmb##sheet##_sheet_alarm, \
      Param::bmb##sheet##_pic_pgm, \
      Param::bmb##sheet##_pic_pgc, \
      Param::bmb##sheet##_pic_pgd, \
      Param::bmb##sheet##_alarm_reason, \
      Param::bmb##sheet##_alarm_brick, \
   }

static const RoadsterBmb::SheetParams sheetParams[RoadsterBmb::NumSheets] =
{
   SHEET_PARAMS(1),
   SHEET_PARAMS(2),
   SHEET_PARAMS(3),
   SHEET_PARAMS(4),
   SHEET_PARAMS(5),
   SHEET_PARAMS(6),
   SHEET_PARAMS(7),
   SHEET_PARAMS(8),
   SHEET_PARAMS(9),
   SHEET_PARAMS(10),
   SHEET_PARAMS(11),
};
static const int RoadsterBricksPerSheet = 9;
static const int RoadsterThermistorsPerSheet = 6;
static const int TotalRoadsterBricks = RoadsterBmb::NumSheets * RoadsterBricksPerSheet;
static const int MebThermistors = MebBms::NumCells / 12;
static const int TotalRoadsterThermistors = RoadsterBmb::NumSheets * RoadsterThermistorsPerSheet;

static int MappedCellIndex(int sheet, int brick)
{
   const int virtualBrick = sheet * RoadsterBricksPerSheet + brick;
   // Spread the 99 virtual Roadster brick slots evenly across the 96 MEB cells.
   // Integer division deliberately creates a few duplicated source cells, which
   // is acceptable here because the goal is preserving the original 9-brick
   // Roadster message layout while staying monotonic across the MEB stack.
   return MIN(MebBms::NumCells - 1, (virtualBrick * MebBms::NumCells) / TotalRoadsterBricks);
}

static int MappedThermistorIndex(int sheet, int thermistor)
{
   const int virtualThermistor = sheet * RoadsterThermistorsPerSheet + thermistor;
   // Spread the 66 virtual Roadster thermistor slots evenly across the 8 MEB
   // module temperatures. Integer division intentionally repeats nearby module
   // temperatures so each emitted sheet keeps the original 6-therm format.
   return MIN(MebThermistors - 1, (virtualThermistor * MebThermistors) / TotalRoadsterThermistors);
}

static uint32_t BalanceCanId(int sheet)
{
   return 0x88 + (sheet * 8);
}

static uint32_t VoltageCanId(int sheet)
{
   return 0x8A + (sheet * 8);
}

static uint32_t TemperatureCanId(int sheet)
{
   return 0x8C + (sheet * 8);
}

static uint32_t StatusCanId(int sheet)
{
   return 0x8E + (sheet * 8);
}

struct VersionFrame
{
   uint32_t canId;
   uint8_t len;
   uint8_t data[8];
};

static const uint32_t IdentificationRequestId = 0x000; // VMS identification request observed before the BMB version replay.
static const uint32_t VmsHandshakeId = 0x380; // VMS handshake command carrying the 00 02 08 00 00 10 trigger payload.
// Observed BMB reply to VMS_BMB_Request (02 00 14 00 ..) in logs/26-07-17 Christians Roadster fahrbereit.csv.
static const uint8_t BmbRequestReplyData[] = { 0x09, 0x74, 0x19, 0x29, 0x1B, 0x02 };
// Observed BMB reply to VMS_BMB_Request_Broadcast (02 00 02 00 ..) in logs/26-07-17 Christians Roadster boot.csv.
static const uint8_t BmbBroadcastReplyData[] = { 0x09, 0x46, 0x00, 0x00, 0x00, 0x01 };

// Node IDs used by BMB sheets on the Roadster bus (one per sheet, stride 8).
// Each sheet receives on 0x00A + sheet*8 and replies on 0x30A + sheet*8.
// Cell average voltage replies split across two IDs per sheet:
//   msgIdx 0,1 → 0x308 + sheet*8 (CellAvgReplyBaseId)
//   msgIdx 2   → 0x30A + sheet*8 (NodeReplyBaseId)
static const uint32_t NodeBroadcastId = 0x006;
static const uint32_t NodeBaseId = 0x00A;
static const uint32_t NodeReplyBaseId = 0x30A;
static const uint32_t CellAvgReplyBaseId = 0x308; // base for cell avg msgs 0 and 1
static const uint32_t NodeIdStride = 8;

// Reply to 0x006 [1F 00]: BMB node type/version info.
static const uint8_t BroadcastInfoReply[] = { 0x1F, 0x00, 0x01, 0x14, 0x00, 0x00, 0x0E, 0x00 };
// Reply to 0x006 [24 00 00 01 F0 00]: BMB capability advertisement.
static const uint8_t BroadcastCapabilityReply[] = { 0x24, 0x00, 0x00, 0x02, 0x40, 0x02, 0xFF, 0xFF };
// Reply to 0x006 [0E 00]: disconnect acknowledgement.
static const uint8_t BroadcastDisconnectReply[] = { 0x0E, 0x00, 0x00, 0x82 };
// Reply to directed [23 00]: presence/address acknowledgement.
static const uint8_t Directed23Reply[] = { 0x23, 0x00, 0xE6, 0x00, 0x00, 0x00 };

// Firmware / hardware version string chunks served via directed 0x24 SDO reads.
// Observed in logs/26-07-17 Christians Roadster boot.csv lines 366-535.
struct FirmwareInfoEntry
{
   uint8_t subLo;
   uint8_t subHi;
   uint8_t reply[6]; // bytes 2-7 of the 8-byte reply (bytes 0-1 are always 0x24 0x00)
};
static const FirmwareInfoEntry firmwareInfo[] =
{
   { 0x00, 0x02, { 0x0F, 0x30, 0x35, 0x2D, 0x31, 0x30 } }, // SW version "05-10"
   { 0x06, 0x02, { 0x44, 0x4E, 0x2D, 0x30, 0x30, 0x30 } }, // FW type "DN-000"
   { 0x0C, 0x02, { 0x30, 0x30, 0x30, 0x30, 0xFF, 0xFF } }, // serial number "0000"
   { 0x40, 0x02, { 0x0F, 0x30, 0x36, 0x2D, 0x30, 0x30 } }, // HW version "06-00"
   { 0x46, 0x02, { 0x31, 0x32, 0x34, 0x32, 0x2D, 0x30 } }, // HW version "1242-0"
   { 0x4C, 0x02, { 0x30, 0x20, 0x41, 0x43, 0xFF, 0xFF } }, // HW version "0 AC"
};
// RoadsterBmb::Update() receives rtc_get_counter_val(), so these are RTC seconds.
static const uint32_t VersionBroadcastPeriodSeconds = 60;
static const uint32_t StartupHandshakeDelaySeconds = 2;
static const VersionFrame versionFrames[] =
{
   { 0x601, 8, { 0x42, 0x53, 0x4D, 0x20, 0x52, 0x30, 0x00, 0x40 } },
   { 0x609, 8, { 0x43, 0x53, 0x42, 0x2D, 0x52, 0x32, 0x00, 0x41 } },
   { 0x701, 8, { 0x56, 0x49, 0x4F, 0x2D, 0x52, 0x34, 0x00, 0x60 } },
};

RoadsterBmb::RoadsterBmb(CanHardware* txCan)
   : canHardware(txCan), map0(txCan, false), map1(txCan, false), handshakeState(HandshakeStartup),
     lastVersionBroadcast(0), startupTime(0), identificationPending(false),
     bmbRequestReplyPending(false), bmbBroadcastReplyPending(false),
     broadcastEchoPending(false), broadcastEchoData{0, 0, 0},
     broadcastInfoPending(false), broadcastCapabilityPending(false), broadcastDisconnectPending(false),
     broadcastCellAvgPending(false)
{
   canMaps[0] = &map0;
   canMaps[1] = &map1;
   canHardware->AddCallback(this);
   InitCanMap();

   for (int i = 0; i < NumSheets; i++)
   {
      directedReplies[i].pending = false;
      directedReplies[i].len = 0;
      for (int j = 0; j < 8; j++) directedReplies[i].data[j] = 0;
   }

   HandleClear();
}

void RoadsterBmb::SendAll()
{
   for (int i = 0; i < NumCanMaps; i++)
      canMaps[i]->SendAll();
}

void RoadsterBmb::HandleRx(uint32_t canId, uint32_t data[2], uint8_t dlc)
{
   const uint8_t* bytes = reinterpret_cast<uint8_t*>(data);

   if (canId == IdentificationRequestId && dlc >= 2 && bytes[0] == 0x00 && bytes[1] == 0x04)
   {
      identificationPending = true;
   }
   else if (canId == VmsHandshakeId && dlc >= 7 &&
            bytes[0] == 0x02 && bytes[1] == 0x00 &&
            bytes[2] == 0x14 && bytes[3] == 0x00)
   {
      bmbRequestReplyPending = true;
   }
   else if (canId == VmsHandshakeId && dlc >= 7 &&
            bytes[0] == 0x02 && bytes[1] == 0x00 &&
            bytes[2] == 0x02 && bytes[3] == 0x00)
   {
      bmbBroadcastReplyPending = true;
   }
   else if (canId == VmsHandshakeId && dlc >= 6 &&
            bytes[0] == 0x00 && bytes[1] == 0x02 && bytes[2] == 0x08 &&
            bytes[3] == 0x00 && bytes[4] == 0x00 && bytes[5] == 0x10)
   {
      identificationPending = true;
   }
   else if (canId == NodeBroadcastId)
   {
      // Broadcast to all BMB nodes: queue reply that will be sent on all 0x30X channels.
      if (dlc >= 3 && bytes[0] == 0x1E)
      {
         broadcastEchoData[0] = bytes[0];
         broadcastEchoData[1] = bytes[1];
         broadcastEchoData[2] = bytes[2];
         broadcastEchoPending = true;
      }
      else if (dlc >= 2 && bytes[0] == 0x1F && bytes[1] == 0x00)
      {
         broadcastInfoPending = true;
      }
      else if (dlc >= 6 && bytes[0] == 0x24 && bytes[2] == 0x00 && bytes[3] == 0x01 && bytes[4] == 0xF0)
      {
         broadcastCapabilityPending = true;
      }
      else if (dlc >= 2 && bytes[0] == 0x0E && bytes[1] == 0x00)
      {
         broadcastDisconnectPending = true;
      }
      else if (dlc >= 1 && bytes[0] == 0x25)
      {
        broadcastCellAvgPending = true;
      }
   }
   else
   {
      // Directed command to a specific sheet node.
      for (int sheet = 0; sheet < NumSheets; sheet++)
      {
         if (canId == NodeBaseId + static_cast<uint32_t>(sheet) * NodeIdStride)
         {
            if (!directedReplies[sheet].pending)
            {
               if (dlc >= 2 && bytes[0] == 0x23 && bytes[1] == 0x00)
               {
                  for (unsigned int i = 0; i < sizeof(Directed23Reply); i++)
                     directedReplies[sheet].data[i] = Directed23Reply[i];
                  directedReplies[sheet].len = sizeof(Directed23Reply);
                  directedReplies[sheet].pending = true;
               }
               else if (dlc >= 5 && bytes[0] == 0x24 && bytes[4] == 0xF0)
               {
                  directedReplies[sheet].data[0] = 0x24;
                  directedReplies[sheet].data[1] = 0x00;
                  FillFirmwareReply(bytes[2], bytes[3], directedReplies[sheet].data);
                  directedReplies[sheet].len = 8;
                  directedReplies[sheet].pending = true;
               }
            }
            break;
         }
      }
   }
}

void RoadsterBmb::HandleClear()
{
   canHardware->RegisterUserMessage(IdentificationRequestId);
   canHardware->RegisterUserMessage(VmsHandshakeId);
   canHardware->RegisterUserMessage(NodeBroadcastId);

   for (int i = 0; i < NumSheets; i++)
      canHardware->RegisterUserMessage(NodeBaseId + static_cast<uint32_t>(i) * NodeIdStride);
}

void RoadsterBmb::Update(MebBms& mebBms, uint32_t time)
{
   if (handshakeState == HandshakeStartup && startupTime == 0)
      startupTime = time;

   const bool sendIdentification = identificationPending ||
                                   (handshakeState == HandshakeStartup && (time - startupTime) >= StartupHandshakeDelaySeconds);
   const bool sendVersionOnly = handshakeState == HandshakeIdle && !sendIdentification &&
                                ((time - lastVersionBroadcast) >= VersionBroadcastPeriodSeconds);

   const bool alive = mebBms.Alive(time);

   for (int sheet = 0; sheet < NumSheets; sheet++)
   {
      const SheetParams& params = sheetParams[sheet];

      if (!alive)
      {
         ClearSheet(params, 4);
         continue;
      }

      int validCount = 0;
      int minRaw = 0x7FFFFFFF;
      int maxRaw = 0;
      int minBrick = 0;
      int maxBrick = 0;
      int sumRaw = 0;
      int bleedMask = 0;

      for (int brick = 0; brick < RoadsterBricksPerSheet; brick++)
      {
         const int mebCell = MappedCellIndex(sheet, brick);
         const float cellVoltage = mebBms.GetCellVoltage(mebCell);

         // Treat values below 1 V as invalid/uninitialized cell measurements.
         if (cellVoltage < 1000)
            continue;

         const int rawVoltage = RawVoltage(cellVoltage);
         sumRaw += rawVoltage;
         validCount++;

         if (rawVoltage < minRaw)
         {
            minRaw = rawVoltage;
            minBrick = brick;
         }

         if (rawVoltage > maxRaw)
         {
            maxRaw = rawVoltage;
            maxBrick = brick;
         }

         if (mebBms.GetBalanceFlag(mebCell))
            bleedMask |= 1 << brick;
      }

      if (validCount != RoadsterBricksPerSheet)
      {
         ClearSheet(params, 4);
         continue;
      }

      int minTempRaw = INT_MAX;
      int maxTempRaw = INT_MIN;
      int sumTempRaw = 0;
      int minTherm = 0;
      int maxTherm = 0;

      for (int therm = 0; therm < RoadsterThermistorsPerSheet; therm++)
      {
         const int mebThermistor = MappedThermistorIndex(sheet, therm);
         const int rawTemp = RawTemperature(mebBms.GetModuleTemperature(mebThermistor));

         sumTempRaw += rawTemp;

         if (rawTemp < minTempRaw)
         {
            minTempRaw = rawTemp;
            minTherm = therm;
         }

         if (rawTemp > maxTempRaw)
         {
            maxTempRaw = rawTemp;
            maxTherm = therm;
         }
      }

      int alarmReason = 0;
      int alarmBrick = 0;
      int sheetAlarmOk = 1;
      int overVoltageOk = 1;
      int cellReversalOk = 1;

      if (maxRaw > RawVoltage(4200))
      {
         alarmReason = 5;
         alarmBrick = maxBrick;
         sheetAlarmOk = 0;
         overVoltageOk = 0;
      }
      else if (minRaw < RawVoltage(2500))
      {
         alarmReason = 4;
         alarmBrick = minBrick;
         sheetAlarmOk = 0;
         cellReversalOk = 0;
      }
      else if (maxTempRaw > RawTemperature(60.0f))
      {
         alarmReason = 3;
         alarmBrick = maxTherm;
         sheetAlarmOk = 0;
      }

      Param::SetInt(params.balMinV, minRaw);
      Param::SetInt(params.balMinBrick, minBrick);
      Param::SetInt(params.balMaxV, maxRaw);
      Param::SetInt(params.balMaxBrick, maxBrick);
      Param::SetInt(params.vMin, minRaw);
      Param::SetInt(params.vMax, maxRaw);
      Param::SetInt(params.vSumAvg, sumRaw); // DBC signal "sumAvg" is encoded as the 9-brick voltage sum.
      Param::SetInt(params.vMinBrick, minBrick);
      Param::SetInt(params.vMaxBrick, maxBrick);
      Param::SetInt(params.tMin, minTempRaw);
      Param::SetInt(params.tMax, maxTempRaw);
      Param::SetInt(params.tAvg, sumTempRaw); // DBC signal "avgTemp" is encoded as the 6-therm temperature sum.
      Param::SetInt(params.tMinTherm, minTherm);
      Param::SetInt(params.tMaxTherm, maxTherm);
      Param::SetInt(params.bleed, bleedMask);
      Param::SetInt(params.daisyRx, 1);
      Param::SetInt(params.overV, overVoltageOk);
      Param::SetInt(params.cellReversal, cellReversalOk);
      Param::SetInt(params.canPwrOk, 1);
      Param::SetInt(params.sheetAlarm, sheetAlarmOk);
      Param::SetInt(params.picPgm, 1);
      Param::SetInt(params.picPgc, 1);
      Param::SetInt(params.picPgd, 1);
      Param::SetInt(params.alarmReason, alarmReason);
      Param::SetInt(params.alarmBrick, alarmBrick);
   }

   if (sendIdentification)
   {
      SendIdentification();
      lastVersionBroadcast = time;
      identificationPending = false;
      handshakeState = HandshakeIdle;
   }
   else if (sendVersionOnly)
   {
      SendVersionFrames();
      lastVersionBroadcast = time;
   }

   if (bmbBroadcastReplyPending)
   {
      SendBmbBroadcastReply();
      bmbBroadcastReplyPending = false;
   }

   if (bmbRequestReplyPending)
   {
      SendBmbRequestReply();
      bmbRequestReplyPending = false;
   }

   SendBroadcastReplies();
   SendDirectedReplies();

   if (broadcastCellAvgPending)
   {
      if (alive)
         SendBroadcastCellAvgReplies(mebBms);
      broadcastCellAvgPending = false;
   }
   //SendAll();
}

void RoadsterBmb::SendIdentification()
{
   //SendAll();
   SendVersionFrames();
}

void RoadsterBmb::SendVersionFrames()
{
   for (unsigned int i = 0; i < sizeof(versionFrames) / sizeof(versionFrames[0]); i++)
   {
      uint8_t data[8];

      for (int j = 0; j < 8; j++)
         data[j] = versionFrames[i].data[j];

      canHardware->Send(versionFrames[i].canId, data, versionFrames[i].len);
   }
}

void RoadsterBmb::SendBmbRequestReply()
{
   uint8_t data[sizeof(BmbRequestReplyData)];

   for (unsigned int i = 0; i < sizeof(BmbRequestReplyData); i++)
      data[i] = BmbRequestReplyData[i];

   canHardware->Send(VmsHandshakeId, data, sizeof(BmbRequestReplyData));
}

void RoadsterBmb::SendBmbBroadcastReply()
{
   uint8_t data[sizeof(BmbBroadcastReplyData)];

   for (unsigned int i = 0; i < sizeof(BmbBroadcastReplyData); i++)
      data[i] = BmbBroadcastReplyData[i];

   canHardware->Send(VmsHandshakeId, data, sizeof(BmbBroadcastReplyData));
}

void RoadsterBmb::SendBroadcastReplies()
{
   // Iterate over all 11 sheet reply channels for each pending broadcast type.
   if (broadcastEchoPending)
   {
      for (int sheet = 0; sheet < NumSheets; sheet++)
      {
         uint8_t data[3] = { broadcastEchoData[0], broadcastEchoData[1], broadcastEchoData[2] };
         canHardware->Send(NodeReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride, data, 3);
      }
      broadcastEchoPending = false;
   }

   if (broadcastInfoPending)
   {
      for (int sheet = 0; sheet < NumSheets; sheet++)
      {
         uint8_t data[sizeof(BroadcastInfoReply)];
         for (unsigned int i = 0; i < sizeof(BroadcastInfoReply); i++)
            data[i] = BroadcastInfoReply[i];
         canHardware->Send(NodeReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride, data, sizeof(BroadcastInfoReply));
      }
      broadcastInfoPending = false;
   }

   if (broadcastCapabilityPending)
   {
      for (int sheet = 0; sheet < NumSheets; sheet++)
      {
         uint8_t data[sizeof(BroadcastCapabilityReply)];
         for (unsigned int i = 0; i < sizeof(BroadcastCapabilityReply); i++)
            data[i] = BroadcastCapabilityReply[i];
         canHardware->Send(NodeReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride, data, sizeof(BroadcastCapabilityReply));
      }
      broadcastCapabilityPending = false;
   }

   if (broadcastDisconnectPending)
   {
      for (int sheet = 0; sheet < NumSheets; sheet++)
      {
         uint8_t data[sizeof(BroadcastDisconnectReply)];
         for (unsigned int i = 0; i < sizeof(BroadcastDisconnectReply); i++)
            data[i] = BroadcastDisconnectReply[i];
         canHardware->Send(NodeReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride, data, sizeof(BroadcastDisconnectReply));
      }
      broadcastDisconnectPending = false;
   }
}

void RoadsterBmb::SendDirectedReplies()
{
   for (int sheet = 0; sheet < NumSheets; sheet++)
   {
      if (directedReplies[sheet].pending)
      {
         canHardware->Send(NodeReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride,
                           directedReplies[sheet].data,
                           directedReplies[sheet].len);
         directedReplies[sheet].pending = false;
      }
   }
}

void RoadsterBmb::SendBroadcastCellAvgReplies(MebBms& mebBms)
{
   // For each sheet, send 3 messages of 3 bricks each, covering all 9 bricks.
   // Format per message: [0x20, msgIdx, v0_lo, v0_hi, v1_lo, v1_hi, v2_lo, v2_hi]
   // The Roadster protocol uses two CAN IDs per sheet for these messages:
   //   msgIdx 0,1 → CellAvgReplyBaseId + sheet*8 (= 0x308 + sheet*8)
   //   msgIdx 2   → NodeReplyBaseId    + sheet*8 (= 0x30A + sheet*8)
   static const int BricksPerMsg = 3;
   static const int MsgsPerSheet = (RoadsterBricksPerSheet + BricksPerMsg - 1) / BricksPerMsg; // = 3

   for (int sheet = 0; sheet < NumSheets; sheet++)
   {
      for (int msgIdx = 0; msgIdx < MsgsPerSheet; msgIdx++)
      {
         const uint32_t replyId = (msgIdx < MsgsPerSheet - 1)
            ? (CellAvgReplyBaseId + static_cast<uint32_t>(sheet) * NodeIdStride)
            : (NodeReplyBaseId    + static_cast<uint32_t>(sheet) * NodeIdStride);

         uint8_t data[8];
         data[0] = 0x20; // multiplexer
         data[1] = static_cast<uint8_t>(msgIdx);

         for (int i = 0; i < BricksPerMsg; i++)
         {
            const int brick = msgIdx * BricksPerMsg + i;
            const int mebCell = MappedCellIndex(sheet, brick);
            const int rawV = RawVoltage(mebBms.GetCellVoltage(mebCell));
            data[2 + i * 2]     = static_cast<uint8_t>(rawV & 0xFF);
            data[2 + i * 2 + 1] = static_cast<uint8_t>((rawV >> 8) & 0xFF);
         }

         canHardware->Send(replyId, data, 8);
      }
   }
}

void RoadsterBmb::FillFirmwareReply(uint8_t subLo, uint8_t subHi, uint8_t* buf)
{
   const unsigned int count = sizeof(firmwareInfo) / sizeof(firmwareInfo[0]);
   for (unsigned int i = 0; i < count; i++)
   {
      if (firmwareInfo[i].subLo == subLo && firmwareInfo[i].subHi == subHi)
      {
         for (int j = 0; j < 6; j++)
            buf[2 + j] = firmwareInfo[i].reply[j];
         return;
      }
   }
   // Unknown sub-index: reply with zeroes.
   for (int j = 2; j < 8; j++)
      buf[j] = 0x00;
}

void RoadsterBmb::ClearSheet(const SheetParams& params, int alarmReason)
{
   Param::SetInt(params.balMinV, 0);
   Param::SetInt(params.balMinBrick, 0);
   Param::SetInt(params.balMaxV, 0);
   Param::SetInt(params.balMaxBrick, 0);
   Param::SetInt(params.vMin, 0);
   Param::SetInt(params.vMax, 0);
   Param::SetInt(params.vSumAvg, 0);
   Param::SetInt(params.vMinBrick, 0);
   Param::SetInt(params.vMaxBrick, 0);
   Param::SetInt(params.tMin, 0);
   Param::SetInt(params.tMax, 0);
   Param::SetInt(params.tAvg, 0);
   Param::SetInt(params.tMinTherm, 0);
   Param::SetInt(params.tMaxTherm, 0);
   Param::SetInt(params.bleed, 0);
   Param::SetInt(params.daisyRx, 0);
   Param::SetInt(params.overV, 1);
   Param::SetInt(params.cellReversal, 1);
   Param::SetInt(params.canPwrOk, 0);
   Param::SetInt(params.sheetAlarm, 0);
   Param::SetInt(params.picPgm, 1);
   Param::SetInt(params.picPgc, 1);
   Param::SetInt(params.picPgd, 1);
   Param::SetInt(params.alarmReason, alarmReason);
   Param::SetInt(params.alarmBrick, 0);
}

void RoadsterBmb::InitCanMap()
{
   for (int sheet = 0; sheet < NumSheets; sheet++)
   {
      const SheetParams& params = sheetParams[sheet];
      CanMap& map = MapForSheet(sheet);
      const uint32_t balId = BalanceCanId(sheet);
      const uint32_t voltageId = VoltageCanId(sheet);
      const uint32_t tempId = TemperatureCanId(sheet);
      const uint32_t statusId = StatusCanId(sheet);

      map.AddSend(params.balMinV, balId, 0, 16, 1);
      map.AddSend(params.balMinBrick, balId, 16, 8, 1);
      map.AddSend(params.balMaxV, balId, 24, 16, 1);
      map.AddSend(params.balMaxBrick, balId, 40, 8, 1);

      map.AddSend(params.vMin, voltageId, 0, 16, 1);
      map.AddSend(params.vMax, voltageId, 16, 16, 1);
      map.AddSend(params.vSumAvg, voltageId, 32, 24, 1);
      map.AddSend(params.vMinBrick, voltageId, 56, 4, 1);
      map.AddSend(params.vMaxBrick, voltageId, 60, 4, 1);

      map.AddSend(params.tMin, tempId, 0, 16, 1);
      map.AddSend(params.tMax, tempId, 16, 16, 1);
      map.AddSend(params.tAvg, tempId, 32, 24, 1);
      map.AddSend(params.tMinTherm, tempId, 56, 4, 1);
      map.AddSend(params.tMaxTherm, tempId, 60, 4, 1);

      map.AddSend(params.bleed, statusId, 0, 16, 1);
      map.AddSend(params.daisyRx, statusId, 16, 1, 1);
      map.AddSend(params.overV, statusId, 17, 1, 1);
      map.AddSend(params.cellReversal, statusId, 18, 1, 1);
      map.AddSend(params.canPwrOk, statusId, 19, 1, 1);
      map.AddSend(params.sheetAlarm, statusId, 20, 1, 1);
      map.AddSend(params.picPgm, statusId, 21, 1, 1);
      map.AddSend(params.picPgc, statusId, 22, 1, 1);
      map.AddSend(params.picPgd, statusId, 23, 1, 1);
      map.AddSend(params.alarmReason, statusId, 24, 8, 1);
      map.AddSend(params.alarmBrick, statusId, 32, 8, 1);
   }
}

int RoadsterBmb::RoundToInt(float value)
{
   return (int)std::round(value);
}

int RoadsterBmb::RawVoltage(float cellVoltageMv)
{
   return RoundToInt(cellVoltageMv * 8.192f);
}

int RoadsterBmb::RawTemperature(float temperatureDegC)
{
   return RoundToInt(temperatureDegC * 256.0f);
}

#undef SHEET_PARAMS
