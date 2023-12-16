/*
 * File:   main.cpp
 * Author: Kreyl
 * Project: klNfcF0
 *
 * Created on May 27, 2011, 6:37 PM
 */

#include "ch.h"
#include "hal.h"

#include "kl_lib_f2xx.h"
#include "kl_sd.h"
#include "sound.h"
#include "cmd_uart.h"
#include "ff.h"
#include "MassStorage.h"
#include "evt_mask.h"
#include "main.h"
#include "acc_mma8452.h"
#include "Soundlist.h"
#include "radio_lvl1.h"

#define PAUSE_AFTER_S   18

i2c_t i2c;
SndList_t SndList;
App_t App;

enum State_t { stIdle, stPlaying, stWaiting };
State_t State = stWaiting;

VirtualTimer tmrPauseAfter;

void tmrPauseAfterCB(void* p) {
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_PAUSE_END);
    chSysUnlockFromIsr();
}

void tmrPauseAfter_Restart() {
    chSysLock();
    if(chVTIsArmedI(&tmrPauseAfter)) chVTResetI(&tmrPauseAfter);
    chVTSetI(&tmrPauseAfter, S2ST(PAUSE_AFTER_S), tmrPauseAfterCB, nullptr);
    chSysUnlock();
}

// =============================== Main ========================================
int main() {
#if 1 // ==== Init ====
    // ==== Setup clock ====
    uint8_t ClkResult = FAILURE;
    Clk.SetupFlashLatency(12);  // Setup Flash Latency for clock in MHz
    // 12 MHz/6 = 2; 2*192 = 384; 384/8 = 48 (preAHB divider); 384/8 = 48 (USB clock)
    Clk.SetupPLLDividers(6, 192, pllSysDiv8, 8);
    // 48/4 = 12 MHz core clock. APB1 & APB2 clock derive on AHB clock
    Clk.SetupBusDividers(ahbDiv4, apbDiv1, apbDiv1);
    if((ClkResult = Clk.SwitchToPLL()) == 0) Clk.HSIDisable();
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    App.InitThread();
    Uart.Init(115200);
    Uart.Printf("\rEregionStone   AHB freq=%uMHz\r", Clk.AHBFreqHz/1000000);
    SD.Init();

    i2c.Init(I2C1, GPIOB, 6, 7, 400000, STM32_DMA1_STREAM7, STM32_DMA1_STREAM0);
//    i2c.BusScan();
    Acc.Init();

    // USB related
//    PinSetupIn(PWR_EXTERNAL_GPIO, PWR_EXTERNAL_PIN, pudPullDown);
//    MassStorage.Init();

    Sound.Init();
    Sound.SetVolume(240);
    Sound.RegisterAppThd(chThdSelf());

    Radio.Init();
    // LED
    PinSetupOut(GPIOC, 13, omPushPull);
    PinSet(GPIOC, 13);

    Sound.Play("alive.wav");
    chThdSleepMilliseconds(450);
    PinClear(GPIOC, 13);

    // Report problem with clock if any
    if(ClkResult) Uart.Printf("Clock failure\r");
#endif

    // ==== Main cycle ====
    while(true) {
        eventmask_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        switch(State) {
            case stIdle:
                if(EvtMsk & EVTMSK_ACC_IRQ) {
                    Uart.Printf("AccWhenIdle\r");
                    State = stPlaying;
                    SndList.PlayRandomFileFromDir("Sounds");
                }
                break;

            case stPlaying:
                if(EvtMsk & EVTMSK_PLAY_ENDS) {
                    Uart.Printf("PlayEnd\r");
                    tmrPauseAfter_Restart();
                    State = stWaiting;
                }
                break;

            case stWaiting:
                if(EvtMsk & EVTMSK_ACC_IRQ) {
//                    Uart.Printf("AccWhenW\r");
                    tmrPauseAfter_Restart();
                    PinSet(GPIOC, 13);
                    chThdSleepMilliseconds(18);
                    PinClear(GPIOC, 13);
                }
                else if(EvtMsk & EVTMSK_PAUSE_END) {
                    Uart.Printf("PauseEnd\r");
                    State = stIdle;
                }
                break;
        } // switch

#if 0 // ==== USB connected/disconnected ====
        if(WasExternal and !ExternalPwrOn()) {  // Usb disconnected
            WasExternal = false;
            Usb.Shutdown();
            MassStorage.Reset();
            chSysLock();
            Clk.SetFreq12Mhz();
            Clk.InitSysTick();
            chSysUnlock();
            Uart.Printf("\rUsb Off");
        }
        else if(!WasExternal and ExternalPwrOn()) { // USB connected
            WasExternal = true;
            chSysLock();
            Clk.SetFreq48Mhz();
            Clk.InitSysTick();
            chSysUnlock();
            Usb.Init();
            chThdSleepMilliseconds(540);
            Usb.Connect();
            Uart.Printf("\rUsb On");
        }
#endif

#if 0 // ==== Sensor ====
      if(Sns.CheckEdge() == Rising) {
          if(Sound.State == sndStopped) {
              Uart.Printf("\rDetected");
//              Sound.Play("alive.wav");
              // Generate random
              int32_t i;
              do {
                  uint32_t r = rand() % SndList.ProbSumm + 1; // [1; Probsumm]
                  //uint32_t r = Random(SndList.ProbSumm-1) + 1;
                  Uart.Printf("\rR=%u", r);
                  // Select phrase
                  for(i=0; i<SndList.Count-1; i++) { // do not check last phrase
                      if((r >= SndList.Phrases[i].ProbBottom) and (r <= SndList.Phrases[i].ProbTop)) break;
                  }
              } while(i == PreviousPhrase);
              PreviousPhrase = i;
              // Play phrase
              Sound.Play(SndList.Phrases[i].Filename);
          }
      }
#endif
    } // while true
}

/*
char SndKey[45]="Sound";
uint8_t ReadConfig() {
    int32_t Probability;
    if(SD.iniReadInt32("Sound", "Count", "settings.ini", &SndList.Count) != OK) return FAILURE;
    Uart.Printf("\rCount: %d", SndList.Count);
    if (SndList.Count <= 0) return FAILURE;
    char *c;
    SndList.ProbSumm = 0;
    // Read sounds data
    for(int i=0; i<SndList.Count; i++) {
        // Build SndKey
        c = Convert::Int32ToStr(i+1, &SndKey[5]);   // first symbol after "Sound"
        strcpy(c, "Name");
//        Uart.Printf("\r%s", SndKey);
        // Read filename and probability
        char *S = nullptr;
        if(SD.iniReadString("Sound", SndKey, "settings.ini", &S) != OK) return FAILURE;
        strcpy(SndList.Phrases[i].Filename, S);
        strcpy(c, "Prob");
//        Uart.Printf("\r%s", SndKey);
        if(SD.iniReadInt32 ("Sound", SndKey, "settings.ini", &Probability) != OK) return FAILURE;
        // Calculate probability boundaries
        SndList.Phrases[i].ProbBottom = SndList.ProbSumm;
        SndList.ProbSumm += Probability;
        SndList.Phrases[i].ProbTop = SndList.ProbSumm;
    }
    for(int i=0; i<SndList.Count; i++) Uart.Printf("\r%u %S Bot=%u Top=%u", i, SndList.Phrases[i].Filename, SndList.Phrases[i].ProbBottom, SndList.Phrases[i].ProbTop);
    return OK;
}
*/
