/*
 * KickList.cpp
 *
 *  Created on: 15 θώνÿ 2015 γ.
 *      Author: Kreyl
 */

#include "KickList.h"
#include "main.h"

// Clean timer
void TmrCleanCallback(void *p) {
    chSysLockFromIsr();
    KickList.CleanI();
    chSysUnlockFromIsr();
}

void KickList_t::CleanI() {
    for(uint32_t i=0; i<KICK_BUF_SZ; i++) {
        Buf[i] = 0;
        Intl[i] = 0;
        Cnt = 0;
    }
    Uart.PrintfI("\rClean");
}

void KickList_t::AddI() {
    uint32_t Now = chTimeNow();
    if(Now > Buf[0] + KICK_MIN_INTERVAL) {
        for(uint32_t i=(KICK_BUF_SZ-1); i>0; i--) Buf[i] = Buf[i-1];    // Shift buffer
        Buf[0] = Now;
        Cnt++;
        // Restart cleaning timer
        chVTRestartI(&TmrClean, MS2ST(KICK_CLEAN_DELAY_MS), TmrCleanCallback, nullptr);
        if(Cnt > 1) App.SignalEvtI(EVTMSK_NEW_KICK);
    }
}

uint8_t KickList_t::SearchSeq(uint32_t *PIndx) {    // Zero means nothing
    // Build intervals
    uint32_t n = 0;
    bool First = true;
    for(uint32_t i=(KICK_BUF_SZ-1); i>0; i--) {
        uint32_t t = Buf[i-1] - Buf[i];
        if(t != 0) {
            // Ignore first interval
            if(First) First = false;
            else Intl[n++] = t;
        }
    }
    // Print intervals
    Uart.PrintfI("\rIntls");
    for(uint32_t i=0; i<KICK_BUF_SZ; i++) {
        if(Intl[i] == 0) break;
        Uart.PrintfI(" %u", Intl[i]);
    }
    // Compare sequences
    const uint32_t *p = &ksq[0][0];
    bool IsLike = false;
    for(uint32_t i=0; i<KICK_BUF_SZ; i++) {
        uint32_t t = *p++;
        Uart.PrintfI("\r %u %u", Intl[i], t);
        if(t == KICK_SEQ_END) break;    // End of sequence
        if(Intl[i] == 0) {              // End of buffer
            IsLike = false;
            break;
        }
        if(IS_LIKE(Intl[i], t, KICK_TOLERANCE_MS)) IsLike = true;
        else {                          // Not similar
            IsLike = false;
            break;
        }
    } // For

    if(IsLike) {
        CleanI();
        Uart.PrintfI("\r OK");
        return OK;
    }
    else return FAILURE;
//    return FAILURE;
}
