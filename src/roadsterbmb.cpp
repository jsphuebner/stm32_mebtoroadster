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

static int SheetStartCell(int sheet)
{
   return sheet < 8 ? sheet * 9 : 72 + (sheet - 8) * 8;
}

static int SheetCellCount(int sheet)
{
   return sheet < 8 ? 9 : 8;
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

RoadsterBmb::RoadsterBmb(CanHardware* txCan)
   : map0(txCan, false), map1(txCan, false), map2(txCan, false), map3(txCan, false), map4(txCan, false), map5(txCan, false)
{
   canMaps[0] = &map0;
   canMaps[1] = &map1;
   canMaps[2] = &map2;
   canMaps[3] = &map3;
   canMaps[4] = &map4;
   canMaps[5] = &map5;
   InitCanMap();
}

void RoadsterBmb::SendAll()
{
   for (int i = 0; i < NumCanMaps; i++)
      canMaps[i]->SendAll();
}

void RoadsterBmb::Update(MebBms& mebBms, uint32_t time)
{
   const bool alive = mebBms.Alive(time);

   for (int sheet = 0; sheet < NumSheets; sheet++)
   {
      const SheetParams& params = sheetParams[sheet];
      const int startCell = SheetStartCell(sheet);
      const int cellCount = SheetCellCount(sheet);

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

      for (int cell = 0; cell < cellCount; cell++)
      {
         const float cellVoltage = mebBms.GetCellVoltage(startCell + cell);

         if (cellVoltage < 1000)
            continue;

         const int rawVoltage = RawVoltage(cellVoltage);
         sumRaw += rawVoltage;
         validCount++;

         if (rawVoltage < minRaw)
         {
            minRaw = rawVoltage;
            minBrick = cell;
         }

         if (rawVoltage > maxRaw)
         {
            maxRaw = rawVoltage;
            maxBrick = cell;
         }

         if (mebBms.GetBalanceFlag(startCell + cell))
            bleedMask |= 1 << cell;
      }

      if (validCount == 0)
      {
         ClearSheet(params, 4);
         continue;
      }

      const int startCmu = startCell / 12;
      const int endCmu = (startCell + cellCount - 1) / 12;
      float minTemp = 1000.0f;
      float maxTemp = -1000.0f;
      float avgTemp = 0;
      int tempCount = 0;

      for (int cmu = startCmu; cmu <= endCmu; cmu++)
      {
         const float temp = mebBms.GetModuleTemperature(cmu);
         minTemp = MIN(minTemp, temp);
         maxTemp = MAX(maxTemp, temp);
         avgTemp += temp;
         tempCount++;
      }

      avgTemp /= tempCount;

      const int avgVoltageRaw = sumRaw / validCount;
      const int minTempRaw = RawTemperature(minTemp);
      const int maxTempRaw = RawTemperature(maxTemp);
      const int avgTempRaw = RawTemperature(avgTemp) * RoadsterThermistorsPerSheet;
      int alarmReason = 0;
      int alarmBrick = 0;
      int sheetAlarm = 1;
      int overVoltage = 1;
      int cellReversal = 1;

      if (maxRaw > RawVoltage(4200))
      {
         alarmReason = 5;
         alarmBrick = maxBrick;
         sheetAlarm = 0;
         overVoltage = 0;
      }
      else if (minRaw < RawVoltage(2500))
      {
         alarmReason = 4;
         alarmBrick = minBrick;
         sheetAlarm = 0;
         cellReversal = 0;
      }
      else if (maxTemp > 60.0f)
      {
         alarmReason = 3;
         alarmBrick = maxBrick;
         sheetAlarm = 0;
      }

      Param::SetInt(params.balMinV, minRaw);
      Param::SetInt(params.balMinBrick, minBrick);
      Param::SetInt(params.balMaxV, maxRaw);
      Param::SetInt(params.balMaxBrick, maxBrick);
      Param::SetInt(params.vMin, minRaw);
      Param::SetInt(params.vMax, maxRaw);
      Param::SetInt(params.vSumAvg, avgVoltageRaw * RoadsterBricksPerSheet);
      Param::SetInt(params.vMinBrick, minBrick);
      Param::SetInt(params.vMaxBrick, maxBrick);
      Param::SetInt(params.tMin, minTempRaw);
      Param::SetInt(params.tMax, maxTempRaw);
      Param::SetInt(params.tAvg, avgTempRaw);
      Param::SetInt(params.tMinTherm, 0);
      Param::SetInt(params.tMaxTherm, 0);
      Param::SetInt(params.bleed, bleedMask);
      Param::SetInt(params.daisyRx, 1);
      Param::SetInt(params.overV, overVoltage);
      Param::SetInt(params.cellReversal, cellReversal);
      Param::SetInt(params.canPwrOk, 1);
      Param::SetInt(params.sheetAlarm, sheetAlarm);
      Param::SetInt(params.picPgm, 1);
      Param::SetInt(params.picPgc, 1);
      Param::SetInt(params.picPgd, 1);
      Param::SetInt(params.alarmReason, alarmReason);
      Param::SetInt(params.alarmBrick, alarmBrick);
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
   Param::SetInt(params.cellReversal, 0);
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
   return value >= 0 ? (int)(value + 0.5f) : (int)(value - 0.5f);
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
