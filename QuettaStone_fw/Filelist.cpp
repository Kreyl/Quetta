/*
 * Filelist.cpp
 *
 *  Created on: 20 мая 2016 г.
 *      Author: Kreyl
 */

#include "Filelist.h"
#include "cmd_uart.h"
#include "ff.h"

DIR Dir;
FILINFO FileInfo;

void Filelist_t::ScanDir(const char *DirName) {
    // Count files in the dir
    Count = 0;
    // Open dir
    if(f_opendir(&Dir, DirName) != FR_OK) return;
//    f_chdir(DirName);
    while(true) {
        if(f_readdir(&Dir, &FileInfo) !=  FR_OK) return;  // Get item in dir
        Uart.Printf(">%S; %S; attrib=%X\r", FileInfo.fname, FileInfo.lfname, FileInfo.fattrib);
//        if(FileInfo.fattrib & (AM_HID | AM_DIR)) continue; // Ignore hidden files and dirs
//        else {
//            // Get file extension
//            char *S = strchr(FileInfo.fname, '.');
//            // If there is no extension and no extension required, increase counter
//            if(S == nullptr) {
//                if(*Ext == 0) Count++;
//                else continue;
//            }
//            S++; // Next after '.'
//            S = strstr(Ext, S);
//            size_t len = strlen(S);     // Len of ext
//            if(S == nullptr) continue;  // Extension not found
//            // Check if exact extension
//            char c = *(S + len);   // Next char after found ext
//            Uart.Printf("chr: %c %X\r", c, c);
//            // Check if this char is among delimiting ones
//            if(strchr(FL_DELIMITERS, c) != nullptr) Count++;
//        }
    }
}
