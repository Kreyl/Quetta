/*
 * crc_ccitt.h
 *
 *  Created on: May 5, 2020
 *      Author: layst
 */

#ifndef KL_CRC_H__
#define KL_CRC_H__

#include <inttypes.h>

#define CRC_CCITT16

#define CRC_INITVALUE   0x0000U
#define CRC_POLY        0x1021U

namespace Crc {

uint16_t CalculateCRC16(uint8_t *Buf, uint32_t Len);
uint16_t CalculateCRC16HW(uint8_t *Buf, uint32_t Len);

void StartHW();
void AppendHW(uint8_t b);
uint16_t Get();

void InitHWDMA();
uint16_t CalculateCRC16HWDMA(uint8_t *Buf, uint32_t Len);

void CCITT16_PrintTable();

}

#endif //KL_CRC_H__