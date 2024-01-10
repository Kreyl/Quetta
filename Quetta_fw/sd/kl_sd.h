/*
 * sd.h
 *
 *  Created on: 13.02.2013
 *      Author: kreyl
 */

#ifndef KLSD_H__
#define KLSD_H__

#include "ff.h"
#include "ch.h"
#include "hal.h"
#include "kl_lib.h"
#include "uart.h"

// See SDIO clock divider in halconf.h

class sd_t {
private:
    FATFS SDC_FS;
public:
    bool IsReady;
    void Init();
    void Standby();
    void Resume();
    uint8_t Reconnect();
};

extern sd_t SD;

#endif // KLSD_H__
