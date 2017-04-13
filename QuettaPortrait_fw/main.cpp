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
#include "sound.h"
#include "Soundlist.h"
#include "ws2812b.h"
#include "Effects.h"
#include "SimpleSensors.h"
#include "buttons.h"

#if 1 // =========================== Locals ====================================
App_t App;
SndList_t SndList;
enum State_t { staIdle, staEnteringPass, staTooManyTries, staDoorOpen };
State_t State = staIdle;

TmrKL_t TmrBtnPress(MS2ST(7002), EVT_PRESS_TIMEOUT, tktOneShot);

TmrKL_t TmrDoorOpen(MS2ST(9000), EVT_DOOR_OPEN_END, tktOneShot);

#define PASS_TRY_CNT                3
TmrKL_t TmrTooManyTries(MS2ST(9000), EVT_TOOMANYTRIES_END, tktOneShot);

//LedOnOff_t LedState(LED_PIN);

#define PASS_LEN_MAX    8
enum PassAppendRslt_t { parCorrect, parIncorrect, parEnterMore };

class Pass_t {
private:
    uint32_t CorrectLen = 0, EnteredLen = 0;
    uint8_t CorrectSeq[PASS_LEN_MAX];
    uint8_t EnteredSeq[PASS_LEN_MAX];
public:
    void Enter(uint8_t N) {
        if(EnteredLen < CorrectLen) EnteredSeq[EnteredLen++] = N;
    }

    PassAppendRslt_t Check() {
        PassAppendRslt_t Rslt = parCorrect;
        if(EnteredLen == CorrectLen) {
            for(uint32_t i=0; i<CorrectLen; i++) {
                if(EnteredSeq[i] != CorrectSeq[i]) {
                    Rslt = parIncorrect;
                    break;
                }
            }
            Clear();
            return Rslt;
        }
        else return parEnterMore; // Lengths not equal
    }

    uint8_t AppendCorrect(uint8_t N) {
        if(CorrectLen >= PASS_LEN_MAX) return retvOverflow;
        else CorrectSeq[CorrectLen++] = N;
        return retvOk;
    }

    void Clear() { EnteredLen = 0; }
    void ClearCorrect() { CorrectLen = 0; }

    void PrintEntered() {
        Uart.Printf("Entered: ");
        for(uint32_t i=0; i<EnteredLen; i++) Uart.Printf("%u ", EnteredSeq[i]);
        Uart.Printf("\r");
    }
    void PrintCorrect() {
        Uart.Printf("Correct: ");
        for(uint32_t i=0; i<CorrectLen; i++) Uart.Printf("%u ", CorrectSeq[i]);
        Uart.Printf("\r");
    }

    uint8_t TryCnt;
};

Pass_t Pass;
bool MustKnock = true;
bool SpeakTooManyTriesEnd = false;
uint8_t LedSeq = 0;

void BtnHandler();
void ReadCfg();
#endif

int main() {
    // ==== Setup clock ====
//    Clk.SetHiPerfMode();
    Clk.SetupFlashLatency(16);  // Setup Flash Latency for clock in MHz
    // 12 MHz/12 = 1; 1*192 = 192; 192/6 = 32 (preAHB divider); 192/4 = 48 (USB clock)
    Clk.SetupPllMulDiv(12, 192, pllSysDiv6, 4);
    // 32/2 = 16 MHz core clock. APB1 & APB2 clock derive on AHB clock
    Clk.SetupBusDividers(ahbDiv2, apbDiv1, apbDiv1);
    uint8_t ClkResult = retvFail;
    if((ClkResult = Clk.SwitchToPLL()) == 0) Clk.DisableHSI();

    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();
    App.Init();

    // ==== Init Hard & Soft ====
    Uart.Init(115200, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN);
    Uart.Printf("\r%S %S\r\n", APP_NAME, BUILD_TIME);
    Clk.PrintFreqs();

    chThdSleepMilliseconds(450);

    SD.Init();

    ReadCfg();

    // LEDs
    Effects.Init();
//    Effects.Flashes();
//    Effects.AllTogetherSmoothly(clGreen, 360);
//    Effects.AllTogetherNow(clRed);

//    LedState.Init();
//    LedState.On();

    // Sensors
    SimpleSensors::Init();
    TmrBtnPress.Init();
    TmrDoorOpen.Init();
    TmrTooManyTries.Init();

//    Adc.Init();
//    Adc.EnableVRef();
//    Adc.TmrInitAndStart();

    Sound.Init();
    Sound.SetVolume(255);
    Sound.Play("alive.wav");

    // ==== Main cycle ====
    App.ITask();
}

__noreturn
void App_t::ITask() {
    bool MustCheck = false;
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

        if(Evt & EVT_BUTTONS) {
            TmrBtnPress.Stop();
            BtnEvtInfo_t EInfo;
            if(BtnGetEvt(&EInfo) == retvOk and MustKnock and (State != staDoorOpen)) {
                SndList.PlayRandomFileFromDir("Knock");
                Pass.Enter(EInfo.BtnID+1);
                MustCheck = true;
            }
        }

        if(Evt & EVT_PLAY_ENDS) {
            MustKnock = true;
            if(MustCheck) {
                MustCheck = false;
                BtnHandler();
            }
        }

        if(Evt & EVT_PRESS_TIMEOUT) {
            Uart.Printf("PressTO\r");
            Effects.AllTogetherSmoothly(clRed, 360);
            State = staIdle;
            Pass.Clear();
        }
        if(Evt & EVT_DOOR_OPEN_END) {
            Uart.Printf("DoorOpenTO\r");
            MustKnock = false;
            SndList.PlayRandomFileFromDir("Closing");
            Effects.AllTogetherSmoothly(clGreen, 360);
            State = staIdle;
            Pass.Clear();
        }
        if(Evt & EVT_TOOMANYTRIES_END) {
            Uart.Printf("TriesTO\r");
            Pass.TryCnt = 0;
            if(SpeakTooManyTriesEnd) {
                MustKnock = false;
                SndList.PlayRandomFileFromDir("Ready");
            }
            State = staIdle;
            Pass.Clear();
        }

        if(Evt & EVT_LED_DONE) {
            if(State == staDoorOpen) {
                if(LedSeq == 0) {
                    LedSeq = 1;
                    Effects.AllTogetherSmoothly((Color_t){0, 4, 0}, 1008);
                }
            }
            else {
                if(!Effects.AreOff()) Effects.AllTogetherSmoothly(clBlack, 630);
            }
        }

        if(Evt & EVT_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
    } // while true
}

void BtnHandler() {
    Uart.Printf("Sta %u; ", State);
    Pass.PrintEntered();
    switch(State) {
        case staIdle:
            State = staEnteringPass;
            // No break here intentonally

        case staEnteringPass: {
            PassAppendRslt_t Rslt = Pass.Check();
            switch(Rslt) {
                case parCorrect:
                    TmrTooManyTries.Stop();
                    Pass.TryCnt = 0;
                    Pass.Clear();
                    TmrDoorOpen.StartOrRestart();
                    SndList.PlayRandomFileFromDir("GoodKey");
                    LedSeq = 0;
                    Effects.AllTogetherSmoothly(clGreen, 360);
                    MustKnock = false;
                    State = staDoorOpen;
                    break;

                case parIncorrect:
                    TmrTooManyTries.StartOrRestart();   // Always reset try cnt after a while
                    Pass.TryCnt++;
                    Pass.Clear();
                    if(Pass.TryCnt < PASS_TRY_CNT) {
                        SpeakTooManyTriesEnd = false;
                        SndList.PlayRandomFileFromDir("BadKey");
                        Effects.AllTogetherSmoothly(clRed, 360);
                        MustKnock = false;
                        State = staIdle;
                    }
                    else { // too many tries
                        SpeakTooManyTriesEnd = true;
                        SndList.PlayRandomFileFromDir("TooManyTries");
                        Effects.AllTogetherSmoothly(clRed, 360);
                        MustKnock = false;
                        State = staTooManyTries;
                    }
                    break;

                case parEnterMore:
                    TmrBtnPress.StartOrRestart();
                    break;
            } // switch rslt
        } break;

        case staTooManyTries:
            Pass.Clear();
            SndList.PlayRandomFileFromDir("TooManyTries");
            MustKnock = false;
            break;

        case staDoorOpen:
            Pass.Clear();
            break;
    } // switch(State)
}

void ReadCfg() {
    uint32_t dw32;
    // Read pass
    Pass.ClearCorrect();
    if(SD.iniRead<uint32_t>("config.ini", "Pass", "Count", &dw32) == retvOk) {
//        Uart.Printf("Count: %u\r", Cnt);
        for(u32 i=1; i<=dw32; i++) {
            char KeyName[9] = "Point";
            itoa(i, &KeyName[5], 10);
            if(i < 10) KeyName[6] = 0;
            else KeyName[7] = 0;
            uint8_t Point;
            if(SD.iniRead<uint8_t>("config.ini", "Pass", KeyName, &Point) == retvOk) {
                Pass.AppendCorrect(Point);
            }
            else break;
        }
        Pass.PrintCorrect();
    }

    // Read timings
    if(SD.iniRead<uint32_t>("config.ini", "Timings", "DoorOpen_s", &dw32) == retvOk) {
        TmrDoorOpen.SetNewPeriod_s(dw32);
    }
    if(SD.iniRead<uint32_t>("config.ini", "Timings", "TooManyTries_s", &dw32) == retvOk) {
        TmrTooManyTries.SetNewPeriod_s(dw32);
    }
}

#if 1 // ======================= Command processing ============================
void App_t::OnCmd(Shell_t *PShell) {
    Cmd_t *PCmd = &PShell->Cmd;
    Uart.Printf("\r%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ack(retvOk);

    else if(PCmd->NameIs("Version")) PShell->Printf("%S %S\r", APP_NAME, BUILD_TIME);

    else if(PCmd->NameIs("k")) SndList.PlayRandomFileFromDir("Knock");
    else if(PCmd->NameIs("b")) SndList.PlayRandomFileFromDir("BadKey");
    else if(PCmd->NameIs("c")) SndList.PlayRandomFileFromDir("Closing");
    else if(PCmd->NameIs("g")) SndList.PlayRandomFileFromDir("GoodKey");
    else if(PCmd->NameIs("r")) SndList.PlayRandomFileFromDir("Ready");
    else if(PCmd->NameIs("t")) SndList.PlayRandomFileFromDir("TooManyTries");

    else if(PCmd->NameIs("RGB")) {
        Color_t Clr;
        if(PCmd->GetParams<uint8_t>(3, &Clr.R, &Clr.G, &Clr.B) == retvOk) {
            Effects.AllTogetherNow(Clr);
        }
        else PShell->Ack(retvCmdError);
    }
    else if(PCmd->NameIs("RGBs")) {
        Color_t Clr;
        if(PCmd->GetParams<uint8_t>(3, &Clr.R, &Clr.G, &Clr.B) == retvOk) {
            Effects.AllTogetherSmoothly(Clr, 360);
        }
        else PShell->Ack(retvCmdError);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
