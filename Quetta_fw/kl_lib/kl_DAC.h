/*
 * kl_DAC.h
 *
 *  Created on: 21 мая 2020 г.
 *      Author: layst
 */

#ifndef KL_DAC_H__
#define KL_DAC_H__

#include <inttypes.h>
#include "kl_buf.h"

#define DAC_BUF_EN  FALSE

#if DAC_BUF_EN
#define SAMPLING_FREQ_HZ    100000

#define DAC_BUF_SZ  18000
typedef BufTypeSz_t<uint16_t, DAC_BUF_SZ> DacBuf_t;
#endif

class Dac_t {
private:
#if DAC_BUF_EN
    void FillBuf(volatile DacBuf_t *pBuf);
#endif
public:
#if DAC_BUF_EN
    bool MustMakeNoise = false;
    float Amplitude, fCurr, fEnd, fIncrement;
    void InitDMAAndTmr();
    void DeInitDMAAndTmr();
//    void ConstructSinAndStart(uint32_t FreqHz, uint32_t Amplitude, float Offset = 2048.0);

    void Sweep();
    void OnBufEnd();

    void Stop();
#endif
    void Init();

    void EnableCh1();
    void EnableCh2();
    void DisableCh1();
    void DisableCh2();

    void SetCh1(uint16_t AValue);
    void SetCh2(uint16_t AValue);
};

#endif //KL_DAC_H__