#ifndef ROADSTERBMB_H_INCLUDED
#define ROADSTERBMB_H_INCLUDED

#include "canmap.h"
#include "canhardware.h"
#include "mebbms.h"

class RoadsterBmb : public CanCallback
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
      static const int NumCanMaps = 2;
      explicit RoadsterBmb(CanHardware* txCan);
      void Update(MebBms& mebBms, uint32_t time);
      void SendAll();
      void HandleRx(uint32_t canId, uint32_t data[2], uint8_t dlc) override;
      void HandleClear() override;

   private:
      enum HandshakeState
      {
         HandshakeStartup,
         HandshakeIdle,
      };

      CanHardware* canHardware;
      CanMap map0;
      CanMap map1;
      CanMap* canMaps[NumCanMaps];
      HandshakeState handshakeState;
      uint32_t lastVersionBroadcast;
      uint32_t startupTime;
      bool identificationPending;
      bool bmbRequestReplyPending;
      bool bmbBroadcastReplyPending;

      void ClearSheet(const SheetParams& params, int alarmReason);
      void InitCanMap();
      void SendIdentification();
      void SendVersionFrames();
      void SendBmbRequestReply();
      void SendBmbBroadcastReply();
      CanMap& MapForSheet(int sheet) { return *canMaps[(sheet * NumCanMaps) / NumSheets]; }
      static int RoundToInt(float value);
      static int RawVoltage(float cellVoltageMv);
      static int RawTemperature(float temperatureDegC);
};

#endif // ROADSTERBMB_H_INCLUDED
