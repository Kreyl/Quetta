/*
 * KickList.h
 *
 *  Created on: 14 θώνÿ 2015 γ.
 *      Author: Kreyl
 */

#ifndef KICKLIST_H_
#define KICKLIST_H_

#include "uart.h"

/* ==== Kick sequences ====
      _       _    _                 _
_____| |_____| |__| |_______________| |...
      S   t0    t1 P    t_between    S
*/
#define KICK_MIN_INTERVAL   153     // Shorter intervals ignored
#define KICK_DEADTIME_MS    3600    // Delay after good sequence

#define KICK_TOLERANCE_MS   360     // Radius
#define KICK_LONG_MS        999
#define KICK_SHORT_MS       405
#define KICK_TOO_LONG_MS    2007

#define KICK_BUF_SZ         9

struct ksq_t {
    const int32_t *PSq;
    uint32_t Len;
};
// ==== Table of sequences ====
// Beware! No check is made if buf size is greater than seq length
#define FERRUM
//#define AURUM
//#define ARGENTUM

#ifdef FERRUM
const int32_t ksqA[] = {KICK_LONG_MS, KICK_SHORT_MS, KICK_SHORT_MS};
const int32_t ksqB[] = {KICK_SHORT_MS, KICK_SHORT_MS, KICK_SHORT_MS, KICK_LONG_MS, KICK_LONG_MS};
const int32_t ksqC[] = {KICK_SHORT_MS, KICK_LONG_MS, KICK_SHORT_MS, KICK_LONG_MS, KICK_SHORT_MS};
#elif defined AURUM
const int32_t ksqA[] = {KICK_SHORT_MS, KICK_LONG_MS, KICK_SHORT_MS};
const int32_t ksqB[] = {KICK_SHORT_MS, KICK_SHORT_MS, KICK_SHORT_MS, KICK_LONG_MS, KICK_SHORT_MS};
const int32_t ksqC[] = {KICK_SHORT_MS, KICK_LONG_MS, KICK_LONG_MS, KICK_LONG_MS, KICK_SHORT_MS, KICK_SHORT_MS};
#elif defined ARGENTUM
const int32_t ksqA[] = {KICK_LONG_MS, KICK_LONG_MS, KICK_SHORT_MS};
const int32_t ksqB[] = {KICK_SHORT_MS, KICK_SHORT_MS, KICK_SHORT_MS, KICK_SHORT_MS, KICK_SHORT_MS};
const int32_t ksqC[] = {KICK_SHORT_MS, KICK_LONG_MS, KICK_LONG_MS, KICK_SHORT_MS, KICK_SHORT_MS, KICK_LONG_MS};
#endif

const ksq_t ksqTable[] = {
        {ksqA, countof(ksqA)},
        {ksqB, countof(ksqB)},
        {ksqC, countof(ksqC)},
};
#define KSQ_CNT     countof(ksqTable)


class KickList_t {
private:
    int32_t Buf[KICK_BUF_SZ];
//    uint32_t Intl[KICK_BUF_SZ];
    VirtualTimer TmrDeadtime;
    uint32_t PrevTime = 0;
    bool IsSimilar(int32_t *x, int32_t *y, uint32_t Len, int32_t Radius);
public:
    bool IAppendEnabled = true;
    void AddI();
    uint8_t SearchSeq(int *PIndx);
    void PrintfI() {
        Uart.PrintfI("\rKickList");
        for(uint32_t i=0; i<KICK_BUF_SZ; i++) Uart.PrintfI(" %u", Buf[i]);
    }
};

extern KickList_t KickList;

#endif /* KICKLIST_H_ */
