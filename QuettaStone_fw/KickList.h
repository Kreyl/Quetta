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
#define KICK_DEADTIME_MS    2007    // Delay after good sequence
#define KICK_TOO_LONG_MS    1800    // Consider this as too long interval

#define LONG                1       // }
#define SHRT                0       // } Abstract values

#define KICK_BUF_SZ         9

struct ksq_t {
    const uint32_t *PSq;
    uint32_t Len;
};
// ==== Table of sequences ====
// Beware! No check is made if buf size is greater than seq length
//#define FERRUM
//#define AURUM
#define ARGENTUM

#ifdef FERRUM
const uint32_t ksqA[] = {LONG, SHRT, SHRT};
const uint32_t ksqB[] = {SHRT, SHRT, LONG, LONG, SHRT};
const uint32_t ksqC[] = {SHRT, SHRT, LONG, SHRT, LONG, SHRT};
#elif defined AURUM
const uint32_t ksqA[] = {SHRT, LONG, SHRT};
const uint32_t ksqB[] = {SHRT, LONG, LONG, SHRT, LONG};
const uint32_t ksqC[] = {SHRT, LONG, LONG, LONG, SHRT, SHRT};
#elif defined ARGENTUM
const uint32_t ksqA[] = {LONG, LONG, SHRT};
const uint32_t ksqB[] = {SHRT, SHRT, LONG, SHRT, SHRT};
const uint32_t ksqC[] = {SHRT, LONG, SHRT, LONG, SHRT, LONG};
#endif

const ksq_t ksqTable[] = {
        {ksqA, countof(ksqA)},
        {ksqB, countof(ksqB)},
        {ksqC, countof(ksqC)},
};
#define KSQ_CNT     countof(ksqTable)


class KickList_t {
private:
    uint32_t Buf[KICK_BUF_SZ];
    VirtualTimer TmrDeadtime;
    uint32_t PrevTime = 0;
public:
    bool IAppendEnabled = true;
    void AddI();
    uint8_t SearchSeq(int *PIndx);
};

extern KickList_t KickList;

#endif /* KICKLIST_H_ */
