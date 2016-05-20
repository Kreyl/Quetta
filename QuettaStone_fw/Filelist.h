/*
 * Filelist.h
 *
 *  Created on: 20 мая 2016 г.
 *      Author: Kreyl
 */

#pragma once

#include "kl_sd.h"


#define MAX_EXT_CNT     4

// Delimit with space, comma
// Example: Filelist_t SndList{"mp3, wav"};

#define FL_DELIMITERS   " ,;"

class Filelist_t {
private:
    const char *Ext;
    char PrevName[MAX_NAME_LEN];
public:
    uint32_t Count;
    Filelist_t(const char *Extensions): Ext(Extensions), Count(0) {}
    void Init();
    char CurrentName[MAX_NAME_LEN];
    void ScanDir(const char *DirName);
    uint8_t FindNext();
};
