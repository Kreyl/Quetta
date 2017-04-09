/*
 * File:   main.cpp
 * Author: Elessar
 * Project: MasonOrder
 *
 * Created on May 27, 2016, 6:37 PM
 */

#include "main.h"
#include "kl_lib.h"
#include "led.h"
#include "Sequences.h"
#include "kl_adc.h"
#include "kl_sd.h"

#if 1 // =========================== Locals ====================================
App_t App;

//LedOnOff_t LedState(LED_PIN);

#endif
int main() {
    // ==== Setup clock ====
//    Clk.SetHiPerfMode();
    Clk.SetupFlashLatency(12);  // Setup Flash Latency for clock in MHz
    // 12 MHz/6 = 2; 2*192 = 384; 384/8 = 48 (preAHB divider); 384/8 = 48 (USB clock)
    Clk.SetupPllMulDiv(6, 192, pllSysDiv8, 8);
    // 48/4 = 12 MHz core clock. APB1 & APB2 clock derive on AHB clock
    Clk.SetupBusDividers(ahbDiv4, apbDiv1, apbDiv1);
    uint8_t ClkResult = retvFail;
    if((ClkResult = Clk.SwitchToPLL()) == 0) Clk.DisableHSI();

    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();
    App.InitThread();

    // ==== Init Hard & Soft ====
    Uart.Init(115200, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN);
    Uart.Printf("\r%S %S\r\n", APP_NAME, BUILD_TIME);
    Clk.PrintFreqs();

    SD.Init();
    int32_t Cnt;
    if(SD.iniReadInt32("Sound", "Count", "config.ini", &Cnt) == retvOk) {
        Uart.Printf("\rCount: %d", Cnt);
    }
    // LEDs
//    LedState.Init();
//    LedState.On();

//    Adc.Init();
//    Adc.EnableVRef();
//    Adc.TmrInitAndStart();

    // ==== Main cycle ====
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        uint32_t Evt = chEvtWaitAny(ALL_EVENTS);

#if ADC_REQUIRED
        if(Evt & EVT_SAMPLING) Adc.StartMeasurement();
        if(Evt & EVT_ADC_DONE) {
            if(Adc.FirstConversion) Adc.FirstConversion = false;
            else {
//                uint32_t VBat_adc = Adc.GetResult(ADC_CHNL_BATTERY);
                uint32_t VRef_adc = Adc.GetResult(ADC_CHNL_VREFINT);
//                uint32_t Vbat_mv = (156 * Adc.Adc2mV(VBat_adc, VRef_adc)) / 56;   // Resistor divider 56k & 100k
//                Uart.Printf("VBat_adc: %u; Vref_adc: %u; VBat_mv: %u\r", VBat_adc, VRef_adc, Vbat_mv);
                uint32_t VLr[LR_CNT];
                bool Ready = false;
                for(int i=0; i<LR_CNT; i++) {
                    uint32_t v = Adc.GetResult(AdcChannels[i]);
                    v = Adc.Adc2mV(v, VRef_adc);
                    Filt[i].Put(v);
                    if(Filt[i].IsReady()) {
                        VLr[i] = Filt[i].GetResult();
                        Filt[i].Flush();
                        Ready = true;
                    }
                }

                if(Ready) Uart.Printf("%u %u %u %u   %u %u %u %u\r",
                        VLr[0],VLr[1],VLr[2],VLr[3], VLr[4],VLr[5],VLr[6],VLr[7]);

//                int32_t Vbat_mv = (2 * Adc.Adc2mV(VBat_adc, VRef_adc));   // Resistor divider
//                if(Vbat_mv < 3500) SignalEvt(EVT_BATTERY_LOW);
            } // if not big diff
        } // evt
#endif

        if(Evt & EVT_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
    } // while true
}

#if 1 // ======================= Command processing ============================
void App_t::OnCmd(Shell_t *PShell) {
    Cmd_t *PCmd = &PShell->Cmd;
//    Uart.Printf("\r%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ack(retvOk);

    else if(PCmd->NameIs("Version")) PShell->Printf("%S %S\r", APP_NAME, BUILD_TIME);

    else PShell->Ack(retvCmdUnknown);
}
#endif
