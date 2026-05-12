#ifndef ROADSTERBMB_H_INCLUDED
#define ROADSTERBMB_H_INCLUDED

#include "canmap.h"
#include "mebbms.h"

class RoadsterBmb
{
   public:
      struct SheetParams
      {
         Param::PARAM_NUM balMinV;
         Param::PARAM_NUM balMinBrick;
         Param::PARAM_NUM balMaxV;
         Param::PARAM_NUM balMaxBrick;
         Param::PARAM_NUM vMin;
         Param::PARAM_NUM vMax;
         Param::PARAM_NUM vSumAvg;
         Param::PARAM_NUM vMinBrick;
         Param::PARAM_NUM vMaxBrick;
         Param::PARAM_NUM tMin;
         Param::PARAM_NUM tMax;
         Param::PARAM_NUM tAvg;
         Param::PARAM_NUM tMinTherm;
         Param::PARAM_NUM tMaxTherm;
         Param::PARAM_NUM bleed;
         Param::PARAM_NUM daisyRx;
         Param::PARAM_NUM overV;
         Param::PARAM_NUM cellReversal;
         Param::PARAM_NUM canPwrOk;
         Param::PARAM_NUM sheetAlarm;
         Param::PARAM_NUM picPgm;
         Param::PARAM_NUM picPgc;
         Param::PARAM_NUM picPgd;
         Param::PARAM_NUM alarmReason;
         Param::PARAM_NUM alarmBrick;
      };

      static const int NumSheets = 11;
      static const int NumCanMaps = 6;
      explicit RoadsterBmb(CanHardware* txCan);
      void Update(MebBms& mebBms, uint32_t time);
      void SendAll();

   private:
      CanMap map0;
      CanMap map1;
      CanMap map2;
      CanMap map3;
      CanMap map4;
      CanMap map5;
      CanMap* canMaps[NumCanMaps];

      void ClearSheet(const SheetParams& params, int alarmReason);
      void InitCanMap();
      CanMap& MapForSheet(int sheet) { return *canMaps[sheet / 2]; }
      static int RoundToInt(float value);
      static int RawVoltage(float cellVoltageMv);
      static int RawTemperature(float temperatureDegC);
};

#endif // ROADSTERBMB_H_INCLUDED
