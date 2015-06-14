/*
 * main.h
 *
 *  Created on: 30.03.2013
 *      Author: kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

// External Power Input
#define PWR_EXTERNAL_GPIO   GPIOA
#define PWR_EXTERNAL_PIN    9
static inline bool ExternalPwrOn() { return  PinIsSet(PWR_EXTERNAL_GPIO, PWR_EXTERNAL_PIN); }

// ==== Sound files ====
#define SND_COUNT_MAX   100

struct Snd_t {
     char Filename[MAX_NAME_LEN];
     uint32_t ProbBottom, ProbTop;
};

struct SndList_t {
    Snd_t Phrases[SND_COUNT_MAX];
    int32_t Count;
    int32_t ProbSumm;
};

class App_t {
private:
    Thread *PThread;
    uint8_t ReadConfig();
public:
    VirtualTimer TmrSecond, TmrIndication;
    // Eternal methods
    void InitThread() { PThread = chThdSelf(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
//    void OnUartCmd(Uart_t *PUart);
    // Inner use
    void ITask();
//    App_t(): PThread(nullptr), ID(ID_DEFAULT), Mode(mRxVibro) {}
};

extern App_t App;

#endif /* MAIN_H_ */
