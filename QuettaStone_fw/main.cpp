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

SndList_t SndList;
i2c_t i2c;

// =============================== Main ========================================
int main() {
#if 1 // ==== Init ====
    // ==== Setup clock ====
    Clk.UpdateFreqValues();
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
    Uart.Init(115200);
    SD.Init();

    // USB related
    PinSetupIn(PWR_EXTERNAL_GPIO, PWR_EXTERNAL_PIN, pudPullDown);
    MassStorage.Init();

    Sound.Init();
    Sound.SetVolume(210);
    Sound.RegisterAppThd(chThdSelf());
    Sound.Play("alive.wav");

    // Accelerometer

//    ReadConfig();
    Uart.Printf("\rPortrait   AHB freq=%uMHz", Clk.AHBFreqHz/1000000);
    // Report problem with clock if any
    if(ClkResult) Uart.Printf("Clock failure\r");
#endif

    // ==== Main cycle ====
    bool WasExternal = false;
//    int32_t PreviousPhrase = 0;
    while(true) {
        chThdSleepMilliseconds(306);

#if 1 // ==== USB connected/disconnected ====
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

