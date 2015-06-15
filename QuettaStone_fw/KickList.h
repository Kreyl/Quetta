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
#define KICK_MIN_INTERVAL   153 // Shorter intervals ignored

#define KICK_CLEAN_DELAY_MS 2700

#define KICK_SEQ_CNT        3
#define KICQ_SEQ_SZ         7
#define KICK_SEQ_END        0

#define KICK_TOLERANCE_MS   153  // Plus-minus
const uint32_t ksq[KICK_SEQ_CNT][KICQ_SEQ_SZ] = {
        {950, 450, 450, KICK_SEQ_END},
        {54, 54, 54, 108, KICK_SEQ_END},
        {108, 207, 360, KICK_SEQ_END},
};

#define KICK_BUF_SZ         9
class KickList_t {
private:
    uint32_t Cnt = 0;
    uint32_t Buf[KICK_BUF_SZ];
    uint32_t Intl[KICK_BUF_SZ];
    VirtualTimer TmrClean;
public:
    void AddI();
    void CleanI();
    uint8_t SearchSeq(uint32_t *PIndx);
    void PrintfI() {
        Uart.PrintfI("\rKickList");
        for(uint32_t i=0; i<KICK_BUF_SZ; i++) Uart.PrintfI(" %u", Buf[i]);
    }
};

extern KickList_t KickList;

#endif /* KICKLIST_H_ */
