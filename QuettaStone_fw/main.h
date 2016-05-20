/*
 * main.h
 *
 *  Created on: 30.03.2013
 *      Author: kreyl
 */

#pragma once

#include "kl_sd.h"

// External Power Input
#define PWR_EXTERNAL_GPIO   GPIOA
#define PWR_EXTERNAL_PIN    9
static inline bool ExternalPwrOn() { return  PinIsSet(PWR_EXTERNAL_GPIO, PWR_EXTERNAL_PIN); }

// ==== Sound files ====
class App_t {
private:
    Thread *PThread;
    uint8_t ReadConfig();
public:
    // Eternal methods
    void InitThread() { PThread = chThdSelf(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    // Inner use
    void ITask();
};

extern App_t App;
