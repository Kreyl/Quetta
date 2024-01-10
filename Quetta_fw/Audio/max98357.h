/*
 * max98357.h
 *
 *  Created on: 18 сент. 2022 г.
 *      Author: layst
 */

#ifndef MAX98357_H__
#define MAX98357_H__

#include "kl_lib.h"

struct StereoSample_t {
    int16_t Left, Right;
} __packed;

class max98357_t {
private:
    void DisableSAI() {
        AU_SAI_A->CR1 &= ~SAI_xCR1_SAIEN;
        while(AU_SAI_A->CR1 & SAI_xCR1_SAIEN); // wait until disabled
    }
    void FlushSAI()   { AU_SAI_A->CR2 = SAI_xCR2_FFLUSH; }
public:
    ftVoidVoid SaiDmaCallbackI = nullptr;
    void Init();
    uint8_t SetupSampleRate(uint32_t Fs);
    void EnableSAI()  { AU_SAI_A->CR1 |= SAI_xCR1_SAIEN; }
    void Put(uint16_t Data) { AU_SAI_A->DR = Data; }

    void TransmitBuf(volatile void *Buf, uint32_t Sz16);
    bool IsTransmitting();
    void Stop();

};

extern max98357_t Codec;

#endif // MAX98357_H__
