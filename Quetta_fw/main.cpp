#include "ch.h"
#include "hal.h"
#include "MsgQ.h"
#include "uart2.h"
#include "shell.h"
#include "usb_cdc.h"
#include "led.h"
#include "Sequences.h"

#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
void OnCmd(Shell_t *PShell);
void ITask();
const char* AppVersion = XSTRINGIFY(BUILD_TIME);

CmdUart_t Uart{115200, CMD_UART_PARAMS};
LedBlinker_t Lumos{LED_PIN};

#endif

int main(void) {
//    Iwdg::InitAndStart(4005);
    Clk.SwitchToMSI();
    Clk.SetVoltageRange(mvrHiPerf);
//    Clk.SetupFlashLatency(80, mvrHiPerf);
    Clk.SetupFlashLatency(64, mvrHiPerf);
    // Try quartz
    if(Clk.EnableHSE() == retvOk) {
        Clk.SetupPllSrc(pllsrcHse);
        Clk.SetupM(3); // 12MHz / 3 = 4
//        Clk.SetupPll(40, 2, 2);         // Sys clk: 4 * 40 / 2 => 80
        Clk.SetupPll(32, 2, 2);         // Sys clk: 4 * 32 / 2 => 64
        Clk.SetupPllSai1(24, 2, 2, 7);  // 48Mhz clk: 4 * 24 / 2 => 48
        // Sys clk
        if(Clk.EnablePLL() == retvOk) {
            Clk.EnablePllROut();
            Clk.SwitchToPLL();
        }
        // 48 MHz
        if(Clk.EnablePllSai1() == retvOk) {
            Clk.EnablePllSai1QOut();
            Clk.SetupSai1Qas48MhzSrc();
        }
    }

    Clk.EnablePrefetch();
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

    // Leds
    Lumos.Init();
    Lumos.StartOrRestart(lsqStart);

    // USB
    UsbCDC.Init();
    UsbCDC.Connect();

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {

            case evtIdShellCmdRcvd:
                while(((CmdUart_t*)Msg.Ptr)->GetRcvdCmd() == retvOk) OnCmd((Shell_t*)((CmdUart_t*)Msg.Ptr));
                Lumos.StartOrRestart(lsqCmd);
                break;

            case evtIdUsbCmdRcvd:
                OnCmd((Shell_t*) &UsbCDC);
                UsbCDC.SignalCmdProcessed();
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
