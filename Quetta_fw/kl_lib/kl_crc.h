/*
 * crc_ccitt.h
 *
 *  Created on: May 5, 2020
 *      Author: layst
 */

#ifndef KL_CRC_H_
#define KL_CRC_H_

#include <inttypes.h>
#include "board.h"

// Enable/disable functionality
#define CRC_SW_EN       FALSE // if disabled, only hw will be used

#define CRC_CCITT16

#define CRC_INITVALUE   0x0000U
#define CRC_POLY        0x1021U

namespace Crc {

#if CRC_SW_EN
uint16_t CalculateCRC16(uint8_t *Buf, uint32_t Len, const uint32_t Init = CRC_INITVALUE);
void CCITT16_PrintTable();
#endif

uint16_t CalculateCRC16HW(uint8_t *Buf, uint32_t Len, const uint32_t Init = CRC_INITVALUE);

void StartHW();
void AppendHW(uint8_t b);
void AppendHWBuf(uint8_t *Buf, uint32_t Len);
uint16_t Get();

void Disable();

#ifdef CRC_DMA
uint16_t CalculateCRC16HWDMA(uint8_t *Buf, uint32_t Len);
void StartHWDMA();
void AppendHWDMABuf(uint8_t *Buf, uint32_t Len);
#endif

}

#endif // KL_CRC_H_
