#include "ch.h"
#include "hal.h"
#include "MsgQ.h"
#include "uart.h"
#include "shell.h"
#include "led.h"
#include "Sequences.h"
#include "kl_sd.h"
#include "kl_fs_utils.h"
#include "max98357.h"
#include "AuPlayer.h"
#include "usb_msd.h"

#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
void OnCmd(Shell_t *PShell);
void ITask();
const char* AppVersion = XSTRINGIFY(BUILD_TIME);

static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{CmdUartParams};
PinOutput_t PinAuPwrEn(AU_PWREN);
#endif

#if 1 // ================== Charge, Usb, Battery, Led ==========================
#define BATTERY_LOW_mv  3500
#define BATTERY_DEAD_mv 3200
void OnAdcDoneI();

//const AdcSetup_t AdcSetup = {
//        .VRefBufVoltage = vrefBufDisabled,
//        .SampleTime = ast92d5Cycles,
//        .Oversampling = AdcSetup_t::oversmp128,
//        .DoneCallbackI = OnAdcDoneI,
//        .Channels = {
//                {BAT_ADC_PIN},
//                {nullptr, 0, ADC_VREFINT_CHNL}
//        }
//};

class ChargeUsbLed_t {
private:
    LedSmooth_t Led{LED_PIN};
    bool PinUsbIsHi = false, PinIsChargingWasHi = true;
    uint32_t AutoOffTimeoutSStart = (48UL * 3600UL), AutoOffTimeLeft = AutoOffTimeoutSStart;
public:
    void Init() {
        Led.Init();
        PinSetupInput(IS_CHARGING_PIN, pudPullUp);
        PinSetupInput(USB_DETECT_PIN, pudPullDown);
        // Battery measurement
//        PinSetupOut(BAT_MEAS_EN, omOpenDrain);
//        PinSetLo(BAT_MEAS_EN); // Enable it forever, as 200k produces ignorable current
        // Inner ADC
//        InnAdc.Init(AdcSetup);
//        InnAdc.StartPeriodicMeasurement(1);
    }

//    void SetAutoOffTime(uint32_t AutoOffTimeH) {
//        AutoOffTimeoutSStart = AutoOffTimeH * 3600UL; // Hours to seconds
//        AutoOffTimeLeft = AutoOffTimeoutSStart;
//    }

    void OnSecond(uint32_t VBatRaw, uint32_t VRefRaw) {
        // Check if time to sleep
        /*
        if(AutoOffTimeLeft == 0) {
            Printf("Auto Off\r");
            EnterSleep();
            return;
        }
        else AutoOffTimeLeft--;
        */
        // If USB disconnected, check battery and SD
        if(PinIsLo(USB_DETECT_PIN)) {
            if(PinUsbIsHi) { // Just disconnected
                PinUsbIsHi = false;
                EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbDisconnect));
                Led.StartOrContinue(lsqOk);
            }
            // Check SD
            if(!SD.IsReady) {
                if(SD.Reconnect() == retvOk) Led.StartOrRestart(lsqOk);
                else Led.StartOrContinue(lsqFail);
            }
            // Check battery
            /*
            uint32_t Battery_mV = 2 * InnAdc.Adc2mV(VBatRaw, VRefRaw); // *2 because of resistor divider
//            Printf("%u\r", Battery_mV);
            if(Battery_mV < BATTERY_DEAD_mv) {
                Printf("Discharged: %u\r", Battery_mV);
                EnterSleep();
            }
            */
        }
        // If USB connected, check charging status
        else if(PinIsHi(USB_DETECT_PIN)) {
            if(!PinUsbIsHi) { // Just connected
                PinUsbIsHi = true;
                EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbConnect));
            }
            // Check IsCharging
            if(PinIsLo(IS_CHARGING_PIN)) {
                PinIsChargingWasHi = false;
                Led.StartOrContinue(lsqCharging);
            }
            else { // Is hi => no charging
                if(!PinIsChargingWasHi) { // Just stopped
                    Led.StartOrRestart(lsqOk);
                    PinIsChargingWasHi = true;
                }
            }
        } // if USB is hi
    }

    void IndicateOk()   { Led.StartOrRestart(lsqOk); }
    void IndicateFail() { Led.StartOrRestart(lsqFail); }
} ChargeUsbLed;
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
    PinAuPwrEn.InitAndSetHi();
    ChargeUsbLed.Init();
    Codec.Init();
    AuPlayer.Init();

    SD.Init();
    if(SD.IsReady) {
//        FwUpdater::CheckAndTryToUpdate();
        uint32_t Volume, AutoOffTimeH;
        if(ini::Read<uint32_t>("Settings.ini", "Sound", "Volume", &Volume) != retvOk) Volume = 100;
        if(Volume > VOLUME_MAX) Volume = VOLUME_MAX;
//        if(ini::Read<uint32_t>("Settings.ini", "Common", "AutoOffTimeH", &AutoOffTimeH) != retvOk) AutoOffTimeH = 48;
//        if(AutoOffTimeH == 0) AutoOffTimeH = 1;
//        else if(AutoOffTimeH > 1000000) AutoOffTimeH = 1000000;
//        ChargeUsbLed.SetAutoOffTime(AutoOffTimeH);
        Printf("Volume: %u; AutoOffTime: %u\r", Volume, AutoOffTimeH);
        AuPlayer.Volume = Volume;
//        ChargeUsbLed.IndicateOk();
        UsbMsd.Init();
//        SoundControl.PlayWakeupIntro();
        chThdSleepMilliseconds(99); // Allow it to start
    } // if SD is ready
    else {
        ChargeUsbLed.IndicateFail();
        chThdSleepMilliseconds(3600);
//        EnterSleep();
    }

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
                break;

            case evtIdAudioPlayStop:
                Printf("Snd Done\r");
//                SoundControl.OnSndEnd();
//                if(MustSleep) EnterSleep();
                break;

//            case evtIdDoFade: SoundControl.OnTmrDoFade(); break;

            case evtIdEverySecond:
//                Iwdg::Reload();
//                ChargeUsbLed.OnSecond(Msg.Values16[0], Msg.Values16[1]); // ADC values here
                break;

#if 1 // ======= USB =======
            case evtIdUsbConnect:
                AuPlayer.Stop();
                Printf("USB connect\r");
                UsbMsd.Connect();
                break;
            case evtIdUsbDisconnect:
                Printf("USB disconnect\r");
                UsbMsd.Disconnect();
//                FwUpdater::CheckAndTryToUpdate();
                break;
            case evtIdUsbReady:
                Printf("USB ready\r");
                break;
#endif
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

    else if(PCmd->NameIs("play")) {
        AuPlayer.Play(PCmd->GetNextString(), spmSingle);
    }

    else if(PCmd->NameIs("Stop")) {
        AuPlayer.Stop();
    }

    else if(PCmd->NameIs("Vol")) {
        uint32_t V;
        if(PCmd->GetNext<uint32_t>(&V) != retvOk) return;
        AuPlayer.SetVolume(V);
        PShell->Ok();
    }

    else PShell->CmdUnknown();
}
#endif
