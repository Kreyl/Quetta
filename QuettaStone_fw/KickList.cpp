/*
 * KickList.cpp
 *
 *  Created on: 15 θώνÿ 2015 γ.
 *      Author: Kreyl
 */

#include "KickList.h"
#include "main.h"
#include "math.h"

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
    for(uint32_t i=0; i<KICK_BUF_SZ; i++) Uart.PrintfI(" %d", Buf[i]);
    // Iterate sequences
    bool IsLike;
    int n = KSQ_CNT-1;  // Search complex seq first
    while(n >= 0) {
        uint32_t Len = ksqTable[n].Len;
        if(Len == 0) continue;

        // Normalize input
        bool TooLong = false;
        int32_t Norm[KICK_BUF_SZ];
        int32_t min=0x7FFFFFFF, max=0;
        for(uint32_t i=0; i<Len; i++) {
            if(Buf[i] > KICK_TOO_LONG_MS) {
                TooLong = true;
                break;
            }
            if(Buf[i] > max) max = Buf[i];
            if(Buf[i] < min) min = Buf[i];
        }
        if(TooLong) {
            n--;
            continue;
        }
        int32_t mid = (max+min)/2;
        for(uint32_t i=0; i<Len; i++) {
            if(Buf[i] >= mid) Norm[i] = KICK_LONG_MS;
            else Norm[i] = KICK_SHORT_MS;
        }
        Uart.PrintfI("\rNorm %u: ", n);
        for(uint32_t i=0; i<Len; i++) Uart.PrintfI(" %d", Norm[i]);

        // Reverse sequence
        int32_t RevSeq[KICK_BUF_SZ];
        const int32_t *p = &ksqTable[n].PSq[Len-1];
        for(uint32_t i=0; i<Len; i++) RevSeq[i] = *p--;

        // Compare sequences
        IsLike = IsSimilar(Norm, RevSeq, Len, KICK_TOLERANCE_MS);
        if(IsLike) break;   // Sequence found
        else n--;
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

bool KickList_t::IsSimilar(int32_t *x, int32_t *y, uint32_t Len, int32_t Radius) {
    double dist = 0;
    double diff;
    for(uint32_t i = 0; i < Len; i++) {
        diff = x[i] - y[i];
        dist += diff * diff;
        Uart.PrintfI("\r  x=%d, y=%d, Dist=%d", x[i], y[i], (int32_t)dist);
    }
    dist = sqrt(dist);
    Uart.PrintfI("\rDist=%d", (int32_t)dist);
    return (dist < Radius);
}

