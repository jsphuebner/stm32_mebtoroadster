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
      struct SheetReply
      {
         bool pending;
         uint8_t data[8];
         uint8_t len;
      };

      HandshakeState handshakeState;
      uint32_t lastVersionBroadcast;
      uint32_t startupTime;
      bool identificationPending;
      bool bmbRequestReplyPending;
      bool bmbBroadcastReplyPending;

      // 0x006 broadcast pending replies (sent to all nodes on the next Update() call)
      bool broadcastEchoPending;
      uint8_t broadcastEchoData[3];
      bool broadcastInfoPending;
      bool broadcastCapabilityPending;
      bool broadcastDisconnectPending;
      int broadcastCellAvgPending; // 0x25 -> reply with 0x20 cell voltage messages
      int cellAvgSheetOffset;       // next sheet to send in the current cell-avg reply burst
      uint8_t canMapSendIdx;        // rolling index for spreading CanMap SendByIndex across Update() calls

      // Per-sheet directed pending replies (0x0A-0x5A -> 0x30A-0x35A)
      SheetReply directedReplies[NumSheets];

      void ClearSheet(const SheetParams& params, int alarmReason);
      void InitCanMap();
      void SendBroadcastReplies();
      void SendBroadcastCellAvgReplies(MebBms& mebBms, int startSheet, int numSheets);
      void SendDirectedReplies();
      CanMap& MapForSheet(int sheet) { return *canMaps[(sheet * NumCanMaps) / NumSheets]; }
      static void FillFirmwareReply(uint8_t subLo, uint8_t subHi, uint8_t* buf);
      static int RoundToInt(float value);
      static int RawVoltage(float cellVoltageMv);
      static int RawTemperature(float temperatureDegC);
};

#endif // ROADSTERBMB_H_INCLUDED
