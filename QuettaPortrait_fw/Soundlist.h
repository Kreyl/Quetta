/*
 * Soundlist.h
 *
 *  Created on: 18 ���. 2015 �.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"
#include "kl_sd.h"

class SndList_t {
private:
    char Filename[MAX_NAME_LEN];    // to store name with path
    uint32_t PreviousN;
    DIR Dir;
    FILINFO FileInfo;
    FRESULT CountFilesInDir(const char* DirName, uint32_t *PCnt);
public:
    void PlayRandomFileFromDir(const char* DirName);
};
