/*
 * KickList.cpp
 *
 *  Created on: 15 θώνÿ 2015 γ.
 *      Author: Kreyl
 */

#include "KickList.h"
#include "main.h"

// Clean timer
void TmrDeadtimeCallback(void *p) {
    KickList.IAppendEnabled = true;
}

//void KickList_t::CleanI() {
//    for(uint32_t i=0; i<KICK_BUF_SZ; i++) {
//        Buf[i] = 0;
//        Intl[i] = 0;
//    }
//    Uart.PrintfI("\rClean");
//}

void KickList_t::AddI() {
    if(!IAppendEnabled) return;
    uint32_t t = chTimeNow() - PrevTime;
    if(t > KICK_MIN_INTERVAL) {
        for(uint32_t i=(KICK_BUF_SZ-1); i>0; i--) Buf[i] = Buf[i-1];    // Shift buffer
        Buf[0] = t;
        PrevTime = chTimeNow();
        // Restart cleaning timer
//        chVTRestartI(&TmrClean, MS2ST(KICK_CLEAN_DELAY_MS), TmrCleanCallback, nullptr);
        App.SignalEvtI(EVTMSK_NEW_KICK);
    }
}


uint8_t KickList_t::SearchSeq(int *PIndx) {    // Zero means nothing
    // Print intervals
    Uart.PrintfI("\rBuf");
    for(uint32_t i=0; i<KICK_BUF_SZ; i++) {
//        if(Buf[i] == 0) break;
        Uart.PrintfI(" %u", Buf[i]);
    }
    // Iterate sequences
    bool IsLike;
    int n = KSQ_CNT-1;  // Search complex seq first
    while(n >= 0) {
        uint32_t Len = ksqTable[n].Len;
        if(Len == 0) continue;
        // Compare sequences
        IsLike = false;
        uint32_t *pBuft = Buf;                           // First item of buffer
        const uint32_t *pSeqt = &ksqTable[n].PSq[Len-1]; // Last item of sequence
        for(uint32_t i=0; i<Len; i++) {
            Uart.PrintfI("\r %u: %u %u", n, *pBuft, *pSeqt);
            if(IS_LIKE(*pBuft, *pSeqt, KICK_TOLERANCE_MS)) {
                IsLike = true;
                pBuft++;
                pSeqt--;
            }
            else {
                IsLike = false;
                n--;
                break;
            }
        }
        if(IsLike) break;   // Sequence found
    }

    if(IsLike) {
        Uart.PrintfI("\rOK; %u", n);
        IAppendEnabled = false;
        chVTSetI(&TmrDeadtime, MS2ST(KICK_DEADTIME_MS), TmrDeadtimeCallback, nullptr);
        *PIndx = n;
        return OK;
    }
    else return FAILURE;
}
