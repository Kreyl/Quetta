/*
 * kl_DAC.cpp
 *
 *  Created on: 21 мая 2020 г.
 *      Author: layst
 */

#include "kl_DAC.h"
#include "kl_lib.h"
#include "kl_buf.h"
#include "math.h"
#include "shell.h"
#include "MsgQ.h"

#if DAC_BUF_EN
static const stm32_dma_stream_t *PDma;
static Timer_t SamplingTmr{TMR_DAC_SMPL};

#define DAC_DMA_MODE_IRQ(Chnl) \
            (STM32_DMA_CR_CHSEL(Chnl) | \
            DMA_PRIORITY_MEDIUM | \
            STM32_DMA_CR_MSIZE_HWORD | \
            STM32_DMA_CR_PSIZE_HWORD | \
            STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_CIRC | STM32_DMA_CR_TCIE)

#define DAC_DMA_MODE_NO_IRQ(Chnl) \
            (STM32_DMA_CR_CHSEL(Chnl) | \
            DMA_PRIORITY_MEDIUM | \
            STM32_DMA_CR_MSIZE_HWORD | \
            STM32_DMA_CR_PSIZE_HWORD | \
            STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_CIRC | STM32_DMA_CR_TCIE)

static DacBuf_t IBuf1, IBuf2;
static volatile DacBuf_t *BufToWriteTo = &IBuf1;

static volatile bool SignalBufEnd = false, NewBufIsReady = false;
#endif

void Dac_t::Init() {
    // VRef
    rccEnableAPB2(RCC_APB2ENR_SYSCFGEN, FALSE); // Enable clock for SYSCFG and VREF
    VREFBUF->CSR = 1; // VRefBuf en, = 2.048v
    // DAC
    rccEnableDAC1(FALSE);
    DAC->CR = 0; // Reset value
    DAC->MCR = 0; // Connected to ext pin w buf en
    // Calibrate ch 1
    DAC->CR = DAC_CR_CEN1;
    for(uint32_t i=0; i<0b11111UL; i++) {
        DAC->CCR = (DAC->CCR & ~0b11111UL) | i;
        chThdSleepMilliseconds(1);
        if(DAC->SR & DAC_SR_CAL_FLAG1) break;
    }
    DAC->CR = 0;
    // Calibrate ch 2
    DAC->CR = DAC_CR_CEN2;
    for(uint32_t i=0; i<0b11111UL; i++) {
        DAC->CCR = (DAC->CCR & ~(0b11111UL << 16)) | (i << 16);
        chThdSleepMilliseconds(1);
        if(DAC->SR & DAC_SR_CAL_FLAG2) break;
    }
    DAC->CR = 0;
}

void Dac_t::EnableCh1()  { DAC->CR |=  DAC_CR_EN1; }
void Dac_t::EnableCh2()  { DAC->CR |=  DAC_CR_EN2; }
void Dac_t::DisableCh1() { DAC->CR &= ~DAC_CR_EN1; }
void Dac_t::DisableCh2() { DAC->CR &= ~DAC_CR_EN2; }

void Dac_t::SetCh1(uint16_t AValue) { DAC->DHR12R1 = AValue; }
void Dac_t::SetCh2(uint16_t AValue) { DAC->DHR12R2 = AValue; }

#if DAC_BUF_EN // ======================== DMA and timer =======================
void DmaDACIrq(void *p, uint32_t flags) {
    if(NewBufIsReady) {
        dmaStreamDisable(PDma);
//        PrintfI("B: %S; L: %u\r", (BufToWriteTo == &IBuf2)? "B2" : "B1", BufToWriteTo->Length);
        NewBufIsReady = false;
        if(BufToWriteTo->Length == 0) {
            SamplingTmr.Disable();
            SignalBufEnd = false;
            chSysLockFromISR();
            EvtQMain.SendNowOrExitI(EvtMsg_t(evtIdAudioPlayEnd));
            chSysUnlockFromISR();
        }
        else {
            dmaStreamSetMode(PDma, DAC_DMA_MODE_IRQ(DAC_DMA_CHNL));
            dmaStreamSetMemory0(PDma, BufToWriteTo->Buf);
            dmaStreamSetTransactionSize(PDma, BufToWriteTo->Length);
            dmaStreamEnable(PDma);
            BufToWriteTo = (BufToWriteTo == &IBuf1)? &IBuf2 : &IBuf1;
            BufToWriteTo->Length = 0;
        }
    }
    if(SignalBufEnd) {
//        Dac.FillBuf(BufToWriteTo);
        chSysLockFromISR();
        EvtQMain.SendNowOrExitI(EvtMsg_t(evtIdAudioBufEnd));
        chSysUnlockFromISR();
    }
}

void Dac_t::InitDMAAndTmr() {
    DisableCh1();
    // Enable DAC, enable DMA, TIM7 TRGO evt as trigger, trigger enable
    DAC->CR |= DAC_CR_EN1 | DAC_CR_DMAEN1 | (0b010 << 3) | DAC_CR_TEN1;
    // ==== DMA ====
    PDma = dmaStreamAlloc(DAC_DMA, IRQ_PRIO_HIGH, DmaDACIrq, nullptr);
    dmaStreamSetPeripheral(PDma, &DAC->DHR12R1);
    // ==== Sampling timer ====
    SamplingTmr.Init();
    SamplingTmr.SetUpdateFrequencyChangingTopValue(SAMPLING_FREQ_HZ);
    SamplingTmr.SelectMasterMode(mmUpdate);
}

void Dac_t::DeInitDMAAndTmr() {
    dmaStreamDisable(PDma);
    SamplingTmr.Disable();
    DAC->CR &= ~(DAC_CR_EN1 | DAC_CR_DMAEN1 | (0b010 << 3) | DAC_CR_TEN1);
}

void Dac_t::Sweep() {
    SamplingTmr.Disable();
    dmaStreamDisable(PDma);
    // Enable DAC, enable DMA, TIM7 TRGO evt as trigger, trigger enable
    DAC->CR |= DAC_CR_TEN1;
    // Fill both bufs
    FillBuf(&IBuf1);
    FillBuf(&IBuf2);
    BufToWriteTo = &IBuf2; // After end of buf1, play buf2 and fill buf1
    SignalBufEnd = true;
    dmaStreamSetMode(PDma, DAC_DMA_MODE_IRQ(DAC_DMA_CHNL));
    dmaStreamSetMemory0(PDma, IBuf1.Buf);
    dmaStreamSetTransactionSize(PDma, IBuf1.Length);
    dmaStreamEnable(PDma);
    SamplingTmr.Enable();
}

void Dac_t::OnBufEnd() {
    FillBuf(BufToWriteTo);
}

void Dac_t::Stop() {
    SamplingTmr.Disable();
    SignalBufEnd = false;
    dmaStreamDisable(PDma);
    DAC->CR &= ~(DAC_CR_TEN1 | DAC_CR_TEN2); // Disable trigger
    SetCh1(0);
    SetCh2(0);
}

void Dac_t::FillBuf(volatile DacBuf_t *pBuf) {
//    PrintfI("%f\r", fCurr);
    volatile uint16_t *p = pBuf->Buf;
    if(MustMakeNoise) {
        pBuf->Length = DAC_BUF_SZ;
        int32_t A = (uint32_t)Amplitude;
        for(uint32_t i=0; i<DAC_BUF_SZ; i++) {
            *p++ = (uint16_t)(2048L + Random::Generate(-A, A));
        }
    }
    else { // sweep
        pBuf->Length = 0;
        while(fCurr <= fEnd) { // if top freq reached
            // Number of samples per period
            int32_t NSamples = (float)SAMPLING_FREQ_HZ / fCurr;
            if((pBuf->Length + NSamples) > DAC_BUF_SZ) break; // Do not add data if it will overflow buffer
            // Fill with curr freq
            float Multi = 2.0 * M_PI * fCurr / (float)SAMPLING_FREQ_HZ; // normalize freq
            for(int32_t i=0; i<NSamples; i++) {
                *p++ = (uint16_t)(2048.0 + Amplitude * sinf(Multi * (float)i));
            }
            pBuf->Length += NSamples;
            fCurr += fIncrement; // Increment current freq
        } // while
    }
    NewBufIsReady = true;
}
#endif
