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
#define KICK_TOLERANCE_MS   27
#define KICK_T_BETWEEN_MS   999

#define KICK_SEQ_CNT        3
#define KICQ_SEQ_SZ         7
#define KICK_SEQ_END        0
const uint32_t ksq[KICK_SEQ_CNT][KICQ_SEQ_SZ] = {
        {108, 54, KICK_SEQ_END},
        {54, 54, 54, 108, KICK_SEQ_END},
        {108, 207, 360, KICK_SEQ_END},
};

#define KICK_BUF_SZ         KICQ_SEQ_SZ
class KickList_t {
private:
    uint32_t Buf[KICK_BUF_SZ];
    void Get();
public:
    void AddI() {
        for(uint32_t i=(KICK_BUF_SZ-1); i>0; i++) Buf[i] = Buf[i-1];    // Shift buffer
        Buf[0] = chTimeNow();
    }
    uint32_t Analyze() {    // Zero means nothing
        return 0;
    }
    void PrintfI() { Uart.PrintfI("\rKickList %A", Buf, KICK_BUF_SZ, ' '); }
};



#endif /* KICKLIST_H_ */
