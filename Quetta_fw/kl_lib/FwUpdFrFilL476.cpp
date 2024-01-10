/*
 * FwUpdFrFilL476.cpp
 *
 *  Created on: 11.02.2023 г.
 *      Author: layst
 */

#include "FwUpdFrFilL476.h"
#include "shell.h"
#include "kl_fs_utils.h"
#include "kl_crc.h"
#include "kl_lib.h"

#define HDR_VERSION_SZ     32

struct FwHeader_t {
    char Version[HDR_VERSION_SZ];
    uint32_t BinSz;
    uint16_t BinCrc;
} __attribute__((packed));

#define HEADER_SZ   sizeof(FwHeader_t)

// Constants, see datasheet
#define FLASH_START_ADDR    0x08000000UL
#define FLASH_PAGE_SZ       2048UL // bytes in page, see datasheet
#define PAGE_SZ_WORD64      (FLASH_PAGE_SIZE/8)

#define BUF_SZ              FLASH_PAGE_SZ

static union {
    uint64_t __DDWord64;
    uint8_t Buf[BUF_SZ];
};

static void EraseBank2() {
    // Unlock flash
    chSysLock();
    Flash::LockFlash(); // Otherwise HardFault occurs after flashing and without reset
    Flash::UnlockFlash();
    chSysUnlock();
    // Erase other bank
    while(FLASH->SR & FLASH_SR_BSY); // Wait for flash to become idle
    FLASH->SR |= 0xC3FA; // Clean err flags
    // Select bank to clear
    if(FLASH->OPTR & FLASH_OPTR_BFB2) FLASH->CR |= FLASH_CR_MER1; // if current bank is B, clean A
    else FLASH->CR |= FLASH_CR_MER2; // else clean B
    // Clean it
    FLASH->CR |= FLASH_CR_STRT;
    while(FLASH->SR & FLASH_SR_BSY); // Wait for flash to become idle
    FLASH->CR &= ~(FLASH_CR_MER1 | FLASH_CR_MER2); // Clear MassErase flags
    Flash::LockFlash();
}

static uint8_t Write(uint32_t Addr, uint32_t Sz) {
    // Disable flash and instruction cache
    uint32_t OldFlashReg = FLASH->ACR;
    FLASH->ACR &= ~(FLASH_ACR_ICEN | FLASH_ACR_PRFTEN);

    // Unlock flash
    chSysLock();
    Flash::LockFlash(); // Otherwise HardFault occurs after flashing and without reset
    Flash::UnlockFlash();
    chSysUnlock();

    // Prepare
    uint32_t Address = Addr;
    uint32_t *p32 = (uint32_t*)Buf;
    uint32_t *PEnd = (uint32_t*)(Buf + Sz);
    while(FLASH->SR & FLASH_SR_BSY); // Wait for flash to become idle
    FLASH->SR |= 0xC3FB; // Clean err flags
    uint8_t Rslt = retvOk;
    // ==== Do it ====
    chSysLock();
    FLASH->CR |= FLASH_CR_PG;
    while(p32 < PEnd and Rslt == retvOk) {
        *(volatile uint32_t*)Address = *p32++;
        Address += 4;
        *(volatile uint32_t*)Address = *p32++;
        Address += 4;
        // Wait completion
        while(FLASH->SR & FLASH_SR_BSY);
        if(FLASH->SR & FLASH_SR_EOP) FLASH->SR |= FLASH_SR_EOP;
        // Check for errors
        if(FLASH->SR & 0xC3FB) {
            Rslt = retvFail;
            PrintfI("SR: %X\r", FLASH->SR);
            break;
        }
    }
    FLASH->CR &= ~FLASH_CR_PG;
    chSysUnlock();

    Flash::LockFlash();
    // Reset instruction cache and Restore Flash settings
    FLASH->ACR |= FLASH_ACR_ICRST;
    FLASH->ACR &= ~FLASH_ACR_ICRST;
    FLASH->ACR = OldFlashReg;
    return Rslt;
}

static uint32_t GetFlashStartAddr() {
    uint32_t FlashSz = *(volatile uint16_t*)0x1FFF75E0;
    if(FlashSz == 256) return 0x08020000;
    else if(FlashSz == 512) return 0x08040000;
    else return 0x08080000;
}


namespace FwUpdater {

void PrintCurrBank() {
    Printf("%S\r\n", (FLASH->OPTR & FLASH_OPTR_BFB2)? "BankB" : "BankA");
}

void CheckAndTryToUpdate() {
    uint32_t BytesLeft, CurrAddr;
    uint16_t crc16;
    FwHeader_t Hdr;
    // Search for *.bin file
    if(f_findfirst(&Dir, &FileInfo, "", FILENAME_PATTERN) != FR_OK) {
        Printf("File search fail\r");
        return;
    }
    if(FileInfo.fname[0] == 0) {
        Printf("%S not found\r", FILENAME_PATTERN);
        return;
    }
    Printf("Found: %S\r", FileInfo.fname);
    if(TryOpenFileRead(FileInfo.fname, &CommonFile) != retvOk) return;

    // Read header
    if(TryRead(&CommonFile, &Hdr, HEADER_SZ) != retvOk) {
        Printf("Error reading header\r");
        goto End;
    }
    Printf("New version: %S; Sz: %u; crc: 0x%04X\r", Hdr.Version, Hdr.BinSz, Hdr.BinCrc);

    // Check file's crc
    Crc::StartHWDMA();
    BytesLeft = Hdr.BinSz;
    while(BytesLeft) {
        Iwdg::Reload();
        uint32_t BytesToRead = (BytesLeft > BUF_SZ)? BUF_SZ : BytesLeft;
        if(TryRead(&CommonFile, Buf, BytesToRead) != retvOk) {
            Printf("Read Err %u\r",  BytesToRead);
            goto End;
        }
        Crc::AppendHWDMABuf(Buf, BytesToRead);
        BytesLeft -= BytesToRead;
    }
    crc16 = Crc::Get();
    if(crc16 != Hdr.BinCrc) {
        Printf("File CRC Error: 0x%X 0x%X\r", crc16, Hdr.BinCrc);
        goto End;
    }

    // CRC ok, return to start of bin and prepare to write
    if(f_lseek(&CommonFile, HEADER_SZ) != FR_OK) goto End;
    BytesLeft = Hdr.BinSz;
    EraseBank2();
    CurrAddr = GetFlashStartAddr(); // Get address of Bank2

    // Read file and write flash
    while(BytesLeft) {
        Iwdg::Reload();
        uint32_t BytesToProcess = (BytesLeft > BUF_SZ)? BUF_SZ : BytesLeft;
        if(TryRead(&CommonFile, Buf, BytesToProcess) != retvOk) {
            Printf("Read Err2 %u\r",  BytesToProcess);
            goto End;
        }
//        Printf("A: 0x%X Sz: %u\r", CurrAddr, BytesToProcess);
        if(Write(CurrAddr, BytesToProcess) == retvOk) {
            CurrAddr += BytesToProcess;
            BytesLeft -= BytesToProcess;
            Printf(".");
        }
        else {
            Printf("Write err: Addr=0X%X, Sz=%u\r", CurrAddr, BytesToProcess);
            goto End;
        }
    }
    PrintfEOL();

    // Writing done, check it
    Crc::CalculateCRC16HWDMA((uint8_t*)GetFlashStartAddr(), Hdr.BinSz);
    crc16 = Crc::Get();
    if(crc16 != Hdr.BinCrc) {
        Printf("FLASH CRC Error: 0x%X 0x%X\r", crc16, Hdr.BinCrc);
        goto End;
    }

    // Flash CRC ok, delete file and jum to new app
    CloseFile(&CommonFile);
    f_unlink(FileInfo.fname);
    Iwdg::Reload();
    Flash::ToggleBootBankAndReset();
    // Will never be here
    End:
    CloseFile(&CommonFile);
    Crc::Disable();
}

} // namespace
