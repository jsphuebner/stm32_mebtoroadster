/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
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
#include <stdint.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/iwdg.h>
#include "stm32_can.h"
#include "canmap.h"
#include "cansdo.h"
#include "terminal.h"
#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "hwinit.h"
#include "anain.h"
#include "param_save.h"
#include "my_math.h"
#include "errormessage.h"
#include "printf.h"
#include "stm32scheduler.h"
#include "terminalcommands.h"
#include "sdocommands.h"
#include "mebbms.h"
#include "roadsterbmb.h"
#include "chademo.h"
#include "isashunt.h"

#define PRINT_JSON 0

static Stm32Scheduler* scheduler;
static Stm32Can* bmsCan;
static Stm32Can* bmbCan;
static CanMap* canMap;
static RoadsterBmb* roadsterBmb;
static IsaShunt* isa;
MebBms* mebBms;
static float cdmSoc;

static void CalculateCdmSoc(void)
{
   static float estimatedSoc = 0;
   static int32_t asOffset = 0;
   static uint16_t noCurrentTicks = 0;
   static bool initialized = false;
   const float current = isa->GetValue(IsaShunt::CURRENT) / 1000.0f;

   if (!initialized)
   {
      estimatedSoc = MIN(100.0f, MAX(0.0f, mebBms->EstimateSocFromVoltage()));
      asOffset = isa->GetValue(IsaShunt::AS);
      initialized = true;
   }

   if (ABS(current) > 1.0f)
      noCurrentTicks = 0;
   else if (noCurrentTicks < UINT16_MAX)
      noCurrentTicks++;

   if (noCurrentTicks >= 1800) // 3 minutes at 100 ms task rate
   {
      estimatedSoc = MIN(100.0f, MAX(0.0f, mebBms->EstimateSocFromVoltage()));
      asOffset = isa->GetValue(IsaShunt::AS);
      cdmSoc = estimatedSoc;
   }
   else
   {
      const int32_t as = isa->GetValue(IsaShunt::AS) - asOffset;
      const float ah = as / 3600.0f;
      const float maxAh = MAX(1.0f, mebBms->GetMaximumAmpHours());
      const float soc = estimatedSoc + (100.0f * ah / maxAh);
      cdmSoc = MIN(100.0f, MAX(0.0f, soc));
   }
}

static void Ms500Task()
{
   roadsterBmb->SendAll();
}

//sample 100ms task
static void Ms100Task(void)
{
   //The following call toggles the LED output, so every 100ms
   //The LED changes from on to off and back.
   //Other calls:
   //DigIo::led_out.Set(); //turns LED on
   //DigIo::led_out.Clear(); //turns LED off
   //For every entry in digio_prj.h there is a member in DigIo
   DigIo::LedOut.Toggle();
   //The boot loader enables the watchdog, we have to reset it
   //at least every 2s or otherwise the controller is hard reset.
   iwdg_reset();
   //Calculate CPU load. Don't be surprised if it is zero.
   float cpuLoad = scheduler->GetCpuLoad();
   //This sets a fixed point value WITHOUT calling the parm_Change() function
   Param::SetFloat(Param::cpuload, cpuLoad / 10);
   isa->InitializeAndStartIfNeeded();
   CalculateCdmSoc();

   canMap->SendAll();

   ErrorMessage::SetTime(rtc_get_counter_val());

   mebBms->Accumulate();
   roadsterBmb->Update(*mebBms, rtc_get_counter_val());
   ChaDeMo::UpdateParams(*mebBms, cdmSoc);
}

/** This function is called when the user changes a parameter */
void Param::Change(Param::PARAM_NUM paramNum)
{
   switch (paramNum)
   {
   case Param::canspeed:
      if (nullptr != bmsCan)
         bmsCan->SetBaudrate((CanHardware::baudrates)Param::GetInt(Param::canspeed));
      if (nullptr != bmbCan)
         bmbCan->SetBaudrate((CanHardware::baudrates)Param::GetInt(Param::canspeed));
      break;
   case Param::ahmax:
      if (nullptr != mebBms)
         mebBms->SetMaximumAmpHours(Param::GetFloat(Param::ahmax));
      break;
   case Param::chargekp:
   case Param::chargeki:
      if (nullptr != mebBms)
         mebBms->SetControllerGains(Param::GetInt(Param::chargekp), Param::GetInt(Param::chargeki));
      break;
   default:
      //Handle general parameter changes here. Add paramNum labels for handling specific parameters
      break;
   }
}

//Whichever timer(s) you use for the scheduler, you have to
//implement their ISRs here and call into the respective scheduler
extern "C" void tim2_isr(void)
{
   scheduler->Run();
}

int main(void)
{
   extern const TERM_CMD termCmds[];

   clock_setup(); //Must always come first
   rtc_setup();
   ANA_IN_CONFIGURE(ANA_IN_LIST);
   DIG_IO_CONFIGURE(DIG_IO_LIST);
   AnaIn::Start(); //Starts background ADC conversion via DMA
   write_bootloader_pininit(); //Instructs boot loader to initialize certain pins

   tim_setup(); //Sample init of a timer
   nvic_setup(); //Set up some interrupts
   parm_load(); //Load stored parameters

   Stm32Scheduler s(TIM2); //We never exit main so it's ok to put it on stack
   scheduler = &s;
   //Initialize CAN1, including interrupts. Clock must be enabled in clock_setup()
   Stm32Can c(CAN1, CanHardware::Baud500);
   Stm32Can c2(CAN2, CanHardware::Baud125);
   CanMap cm(&c);
   CanSdo sdo(&c, &cm);
   sdo.SetNodeId(33); //Set node ID for SDO access e.g. by wifi module
   MebBms meb(&c);
   IsaShunt i(&c, IsaShunt::CURRENT | IsaShunt::AS);
   ChaDeMo chademo(&c);
   RoadsterBmb roadster(&c2);
   //store a pointer for easier access
   bmsCan = &c;
   bmbCan = &c2;
   canMap = &cm;
   mebBms = &meb;
   isa = &i;
   roadsterBmb = &roadster;
   mebBms->SetMaximumAmpHours(Param::GetFloat(Param::ahmax));
   mebBms->SetControllerGains(Param::GetInt(Param::chargekp), Param::GetInt(Param::chargeki));

   //Restore default CHaDeMo CAN mappings if user erased them
   ChaDeMo::CheckAndRestoreCanMap(&cm);

   //This is all we need to do to set up a terminal on USART3
   Terminal t(USART3, termCmds);
   TerminalCommands::SetCanMap(canMap);

   //Up to four tasks can be added to each timer scheduler
   //AddTask takes a function pointer and a calling interval in milliseconds.
   //The longest interval is 655ms due to hardware restrictions
   //You have to enable the interrupt (int this case for TIM2) in nvic_setup()
   //There you can also configure the priority of the scheduler over other interrupts
   s.AddTask(Ms100Task, 100);
   s.AddTask(Ms500Task, 500);

   //backward compatibility, version 4 was the first to support the "stream" command
   Param::SetInt(Param::version, 4);
   Param::Change(Param::PARAM_LAST); //Call callback one for general parameter propagation

   //Now all our main() does is running the terminal
   //All other processing takes place in the scheduler or other interrupt service routines
   //The terminal has lowest priority, so even loading it down heavily will not disturb
   //our more important processing routines.
   while(1)
   {
      char c = 0;

      t.Run();

      if (sdo.GetPrintRequest() == PRINT_JSON)
      {
         TerminalCommands::PrintParamsJson(&sdo, &c);
      }
   }


   return 0;
}
