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

static const uint32_t IdentificationRequestId = 0x000;
static const uint32_t VmsHandshakeId = 0x380;
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
     lastVersionBroadcast(0), startupTime(0), identificationPending(false)
{
   canMaps[0] = &map0;
   canMaps[1] = &map1;
   canHardware->AddCallback(this);
   InitCanMap();
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
   else if (canId == VmsHandshakeId && dlc >= 6 &&
            bytes[0] == 0x00 && bytes[1] == 0x02 && bytes[2] == 0x08 &&
            bytes[3] == 0x00 && bytes[4] == 0x00 && bytes[5] == 0x10)
   {
      identificationPending = true;
   }
}

void RoadsterBmb::HandleClear()
{
   canHardware->RegisterUserMessage(IdentificationRequestId);
   canHardware->RegisterUserMessage(VmsHandshakeId);
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
}

void RoadsterBmb::SendIdentification()
{
   SendAll();
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
