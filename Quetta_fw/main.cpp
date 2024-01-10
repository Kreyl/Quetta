#include "ch.h"
#include "hal.h"
#include "MsgQ.h"
#include "uart.h"
#include "shell.h"
#include "led.h"
#include "Sequences.h"
#include "kl_sd.h"
#include "kl_fs_utils.h"

#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
void OnCmd(Shell_t *PShell);
void ITask();
const char* AppVersion = XSTRINGIFY(BUILD_TIME);

static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{CmdUartParams};
LedBlinker_t Lumos{LED_PIN};

PinOutput_t PinAuPwrEn(AU_PWREN);

#endif

int main(void) {
    // Start Watchdog. Will reset in main thread by periodic 1 sec events.
//    Iwdg::InitAndStart(4500);
//    Iwdg::DisableInDebug();
#if 1 // ==== Clk, Os, EvtQ, Uart ====
    Clk.SwitchToMSI();
    Clk.SetVoltageRange(mvrHiPerf);
    Clk.SetupFlashLatency(40, mvrHiPerf);
    Clk.EnablePrefetch();
    // HSE or MSI
    if(Clk.EnableHSE() == retvOk) {
        Clk.SetupPllSrc(pllsrcHse);
        Clk.SetupM(3);
    }
    else { // PLL fed by MSI
        Clk.SetupPllSrc(pllsrcMsi);
        Clk.SetupM(1);
    }
    // SysClock 40MHz
    Clk.SetupPll(20, 2, 4);
    Clk.SetupBusDividers(ahbDiv1, apbDiv1, apbDiv1);
    if(Clk.EnablePll() == retvOk) {
        Clk.EnablePllROut();
        Clk.SwitchToPll();
    }
    // 48MHz clock for USB & 12MHz clock for ADC
    Clk.SetupPllSai1(24, 8, 2, 7); // 4MHz * 24 = 96; R = 96 / 8 = 12, Q = 96 / 2 = 48
    if(Clk.EnablePllSai1() == retvOk) {
        // Setup Sai1R as ADC source
        Clk.EnableSai1ROut();
        uint32_t tmp = RCC->CCIPR;
        tmp &= ~RCC_CCIPR_ADCSEL;
        tmp |= 0b01UL << 28; // SAI1R is ADC clock
        // Setup Sai1Q as 48MHz source
        Clk.EnableSai1QOut();
        tmp &= ~RCC_CCIPR_CLK48SEL;
        tmp |= 0b01UL << 26;
        RCC->CCIPR = tmp;
    }
    Clk.UpdateFreqValues();
    // Init OS
    halInit();
    chSysInit();
    OsIsInitialized = true;

    // ==== Init hardware ====
    EvtQMain.Init();
    Uart.Init();
    Printf("\r%S %S\r", APP_NAME, AppVersion);
    Clk.PrintFreqs();
    if(Clk.GetPllSrc() == pllsrcHse) Printf("Quartz ok\r\n");
    else Printf("No Quartz\r\n");

//    FwUpdater::PrintCurrBank();
#endif

    // Leds
    Lumos.Init();
    Lumos.StartOrRestart(lsqStart);

    PinAuPwrEn.InitAndSetHi();

//    Codec.Init();
//    AuPlayer.Init();
    SD.Init();
    if(SD.IsReady) {
//        FwUpdater::CheckAndTryToUpdate();
//        uint32_t Volume, AutoOffTimeH;
//        if(ini::Read<uint32_t>("Settings.ini", "Common", "Volume", &Volume) != retvOk) Volume = 100;
//        if(Volume > VOLUME_MAX) Volume = VOLUME_MAX;
//        if(ini::Read<uint32_t>("Settings.ini", "Common", "AutoOffTimeH", &AutoOffTimeH) != retvOk) AutoOffTimeH = 48;
//        if(AutoOffTimeH == 0) AutoOffTimeH = 1;
//        else if(AutoOffTimeH > 1000000) AutoOffTimeH = 1000000;
//        ChargeUsbLed.SetAutoOffTime(AutoOffTimeH);
//        Printf("Volume: %u; AutoOffTime: %u\r", Volume, AutoOffTimeH);
//        AuPlayer.Volume = Volume;
//        ChargeUsbLed.IndicateOk();
//        UsbMsd.Init();
//        SoundControl.PlayWakeupIntro();
        chThdSleepMilliseconds(99); // Allow it to start
    } // if SD is ready
    else {
//        ChargeUsbLed.IndicateFail();
        chThdSleepMilliseconds(3600);
//        EnterSleep();
    }

    // USB

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmdRcvd:
                while(((CmdUart_t*)Msg.Ptr)->TryParseRxBuff() == retvOk) OnCmd((Shell_t*)((CmdUart_t*)Msg.Ptr));
                Lumos.StartOrRestart(lsqCmd);
                break;

            case evtIdUsbReady:
                Lumos.StartOrRestart(lsqUsbReady);
                break;

            default: break;
        } // switch
    } // while true
}


#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
    Cmd_t *PCmd = &PShell->Cmd;
//    Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("Version: %S %S\r\n", APP_NAME, AppVersion);

    else PShell->CmdUnknown();
}
#endif
