/*
 * kl_sd.cpp
 *
 *  Created on: 13.02.2013
 *      Author: kreyl
 */

#include "kl_sd.h"
#include "sdc.h"
#include "sdc_lld.h"
#include <string.h>
#include <stdlib.h>
#include "kl_lib.h"
#include "uart.h"

sd_t SD;
extern semaphore_t semSDRW;

void sd_t::Init() {
    IsReady = FALSE;
    // Bus pins
    PinSetupAlterFunc(GPIOC,  8, omPushPull, pudPullUp, AF12); // DAT0
    PinSetupAlterFunc(GPIOC,  9, omPushPull, pudPullUp, AF12); // DAT1
    PinSetupAlterFunc(GPIOC, 10, omPushPull, pudPullUp, AF12); // DAT2
    PinSetupAlterFunc(GPIOC, 11, omPushPull, pudPullUp, AF12); // DAT3
    PinSetupAlterFunc(GPIOC, 12, omPushPull, pudNone,   AF12); // CLK
    PinSetupAlterFunc(GPIOD,  2, omPushPull, pudPullUp, AF12); // CMD
    // Power pin
    PinSetupOut(GPIOC, 4, omPushPull);
    PinSetLo(GPIOC, 4); // Power on
    chThdSleepMilliseconds(270);    // Let power to stabilize

    FRESULT err;
    sdcInit();
    sdcStart(&SDCD1, NULL);
    if(sdcConnect(&SDCD1)) {
        Uart.Printf("SD connect error\r");
        return;
    }
    else {
        Uart.Printf("SD capacity: %u\r", SDCD1.capacity);
    }
    err = f_mount(0, &SDC_FS);
    if(err != FR_OK) {
        Uart.Printf("SD mount error\r");
        sdcDisconnect(&SDCD1);
        return;
    }
    // Init RW semaphore
    chSemObjectInit(&semSDRW, 1);
    IsReady = TRUE;
}

#if 1 // ======================= ini file operations ===========================
// ==== Inner use ====
static inline char* skipleading(char *S) {
    while (*S != '\0' && *S <= ' ') S++;
    return S;
}
static inline char* skiptrailing(char *S, const char *base) {
    while ((S > base) && (*(S-1) <= ' ')) S--;
    return S;
}
static inline char* striptrailing(char *S) {
    char *ptr = skiptrailing(strchr(S, '\0'), S);
    *ptr='\0';
    return S;
}

uint8_t sd_t::iniReadString(const char *AFileName, const char *ASection, const char *AKey, char **PPOutput) {
    FRESULT rslt;
    // Open file
    rslt = f_open(&IFile, AFileName, FA_READ+FA_OPEN_EXISTING);
    if(rslt != FR_OK) {
        if (rslt == FR_NO_FILE) Uart.Printf("%S: not found\r", AFileName);
        else Uart.Printf("%S: openFile error: %u\r", AFileName, rslt);
        return retvFail;
    }
    // Check if zero file
    if(IFile.fsize == 0) {
        f_close(&IFile);
        Uart.Printf("Empty file\r");
        return retvFail;
    }
    // Move through file one line at a time until a section is matched or EOF.
    char *StartP, *EndP = nullptr;
    int32_t len = strlen(ASection);
    do {
        if(f_gets(IStr, SD_STRING_SZ, &IFile) == nullptr) {
            Uart.Printf("iniNoSection %S\r", ASection);
            f_close(&IFile);
            return retvFail;
        }
        StartP = skipleading(IStr);
        if((*StartP != '[') or (*StartP == ';') or (*StartP == '#')) continue;
        EndP = strchr(StartP, ']');
        if((EndP == NULL) or ((int32_t)(EndP-StartP-1) != len)) continue;
    } while (strncmp(StartP+1, ASection, len) != 0);

    // Section found, find the key
    len = strlen(AKey);
    do {
        if(!f_gets(IStr, SD_STRING_SZ, &IFile) or *(StartP = skipleading(IStr)) == '[') {
            Uart.Printf("iniNoKey\r");
            f_close(&IFile);
            return retvFail;
        }
        StartP = skipleading(IStr);
        if((*StartP == ';') or (*StartP == '#')) continue;
        EndP = strchr(StartP, '=');
        if(EndP == NULL) continue;
    } while(((int32_t)(skiptrailing(EndP, StartP)-StartP) != len or strncmp(StartP, AKey, len) != 0));
    f_close(&IFile);

    // Process Key's value
    StartP = skipleading(EndP + 1);
    // Remove a trailing comment
    uint8_t isstring = 0;
    for(EndP = StartP; (*EndP != '\0') and (((*EndP != ';') and (*EndP != '#')) or isstring) and ((uint32_t)(EndP - StartP) < SD_STRING_SZ); EndP++) {
        if (*EndP == '"') {
            if (*(EndP + 1) == '"') EndP++;     // skip "" (both quotes)
            else isstring = !isstring; // single quote, toggle isstring
        }
        else if (*EndP == '\\' && *(EndP + 1) == '"') EndP++; // skip \" (both quotes)
    } // for
    *EndP = '\0';   // Terminate at a comment
    striptrailing(StartP);
    *PPOutput = StartP;
    return retvOk;
}
#endif

// ============================== Hardware =====================================
#ifdef __cplusplus
extern "C" {
#endif

bool sdc_lld_is_card_inserted(SDCDriver *sdcp) { return TRUE; }

bool sdc_lld_is_write_protected(SDCDriver *sdcp) { return FALSE; }

#ifdef __cplusplus
}
#endif
