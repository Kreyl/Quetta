/*
 * max98357.cpp
 *
 *  Created on: 18.09.2022 г.
 *      Author: layst
 */

#include "max98357.h"
#include "shell.h"

static const PinOutput_t PinSDMODE(AU_SDMODE);
static const stm32_dma_stream_t *PDmaTx;

max98357_t Codec;
/*
The MAX98357A indicates the left channel word when LRCLK is low
Incoming serial data is always clocked-in on the rising edge of BCLK
LRCLK ONLY supports 8kHz, 16kHz, 32kHz, 44.1kHz, 48kHz, 88.2kHz and 96kHz
*/

#if 1 // =========================== SAI defins ================================
#define SAI_IRQ_NUMBER          74
#define SAI_IRQ_HANDLER         Vector168

#define SAI_FIFO_THR_EMPTY      0
#define SAI_FIFO_THR_1_4        1
#define SAI_FIFO_THR_1_2        2
#define SAI_FIFO_THR_3_4        3
#define SAI_FIFO_THR_FULL       4
#define SAI_FIFO_THR            SAI_FIFO_THR_1_2

#define SAI_CR1_DATASZ_8BIT     ((uint32_t)(0b010 << 5))
#define SAI_CR1_DATASZ_10BIT    ((uint32_t)(0b011 << 5))
#define SAI_CR1_DATASZ_16BIT    ((uint32_t)(0b100 << 5))
#define SAI_CR1_DATASZ_20BIT    ((uint32_t)(0b101 << 5))
#define SAI_CR1_DATASZ_24BIT    ((uint32_t)(0b110 << 5))
#define SAI_CR1_DATASZ_32BIT    ((uint32_t)(0b111 << 5))

#define SAI_SYNC_ASYNC          ((uint32_t)(0b00 << 10))
#define SAI_SYNC_INTERNAL       ((uint32_t)(0b01 << 10))

#define SAI_RISING_EDGE         ((uint32_t)(1 << 9)) // TX changes on falling, RX sampled on rising
#define SAI_FALLING_EDGE        ((uint32_t)(0 << 9)) // TX changes on rising, RX sampled at falling

// Slots related
#define SAI_SLOT_CNT            2
#define SAI_SlotActive_0        (1 << 16)
#define SAI_SlotActive_1        (1 << 17)
#define SAI_SLOTSZ_EQ_DATASZ    (0b00 << 6)
#define SAI_SLOTSZ_16bit        (0b01 << 6)
#define SAI_SLOTSZ_32bit        (0b10 << 6)

#define SAI_MASTER_TX           ((uint32_t)0x00000000)
#define SAI_MASTER_RX           (SAI_xCR1_MODE_0)
#define SAI_SLAVE_TX            (SAI_xCR1_MODE_1)
#define SAI_SLAVE_RX            (SAI_xCR1_MODE_1 | SAI_xCR1_MODE_0)

#define SAI_DMATX_MONO_MODE  \
                        STM32_DMA_CR_CHSEL(SAI_DMA_CHNL) |   \
                        DMA_PRIORITY_MEDIUM | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |     /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_M2P |  /* Direction is memory to peripheral */ \
                        STM32_DMA_CR_TCIE       /* Enable Transmission Complete IRQ */

#define SAI_DMATX_STEREO_MODE  \
                        STM32_DMA_CR_CHSEL(SAI_DMA_CHNL) |   \
                        DMA_PRIORITY_MEDIUM | \
                        STM32_DMA_CR_MSIZE_WORD | \
                        STM32_DMA_CR_PSIZE_WORD | \
                        STM32_DMA_CR_MINC |     /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_M2P |  /* Direction is memory to peripheral */ \
                        STM32_DMA_CR_TCIE       /* Enable Transmission Complete IRQ */

#define SAI_DMARX_MODE  STM32_DMA_CR_CHSEL(Chnl) |   \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_BYTE | \
                        STM32_DMA_CR_PSIZE_BYTE | \
                        STM32_DMA_CR_MINC |         /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |      /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_CIRC           /* Circular buffer enable */
//                        STM32_DMA_CR_TCIE           /* Enable Transmission Complete IRQ */
#endif

// DMA Tx Completed IRQ
extern "C"
void DmaSAITxIrq(void *p, uint32_t flags) {
    chSysLockFromISR();
    if(Codec.SaiDmaCallbackI) Codec.SaiDmaCallbackI();
    chSysUnlockFromISR();
}

void max98357_t::Init() {
    PinSDMODE.Init();
    PinSDMODE.SetHi(); // Enable device, use Left channel

    chThdSleepMilliseconds(18);
#if 1 // ======= Setup SAI =======
    // === Clock ===
    Clk.EnablePllSai2POut(); // Pll2 P Output is utilized as SAI clk
    AU_SAI_RccEn();
    RCC->CCIPR &= ~RCC_CCIPR_SAI1SEL;   // }
    RCC->CCIPR |= 0b01UL << 22;         // } SAI input frequency is PLLSAI2_P

    // === GPIOs ===
    PinSetupAlterFunc(AU_LRCK); // Left/Right (Frame sync) clock output
    PinSetupAlterFunc(AU_SCLK); // Bit clock output
    PinSetupAlterFunc(AU_SDIN); // SAI_A is Master Transmitter

    // === Setup SAI_A as async Master Transmitter ===
    DisableSAI();   // All settings must be changed when both blocks are disabled
    AU_SAI->GCR = 0;    // No external sync input/output
    // Stereo mode, Async, MSB first, Rising edge, Data Sz = 16bit, Free protocol, Master Tx
    AU_SAI_A->CR1 = SAI_SYNC_ASYNC | SAI_RISING_EDGE | SAI_CR1_DATASZ_16BIT | SAI_MASTER_TX;
    // FIFO Threshold = 0, flush FIFO
    AU_SAI_A->CR2 = SAI_xCR2_FFLUSH | SAI_FIFO_THR;
    // Offset 1 bit before Slot0, FS Active Low, FS is start and LeftRight, FS Active Lvl Len = 16, Frame Len = 32
    AU_SAI_A->FRCR = SAI_xFRCR_FSOFF | SAI_xFRCR_FSDEF | ((16 - 1) << 8) | (32 - 1);
    // 0 & 1 slots en, N slots = 2, slot size = data size in CR1, no offset
    AU_SAI_A->SLOTR = SAI_SlotActive_0 | SAI_SlotActive_1 | ((SAI_SLOT_CNT - 1) << 8) | SAI_SLOTSZ_EQ_DATASZ;
    AU_SAI_A->IMR = 0;  // No irq on TX
#endif

#if 1 // ==== DMA ====
    AU_SAI_A->CR1 |= SAI_xCR1_DMAEN;
    PDmaTx = dmaStreamAlloc(SAI_DMA_A, IRQ_PRIO_MEDIUM, DmaSAITxIrq, nullptr);
    dmaStreamSetPeripheral(PDmaTx, &AU_SAI_A->DR);
#endif
}

void max98357_t::TransmitBuf(volatile void *Buf, uint32_t Sz16) {
    dmaStreamDisable(PDmaTx);
    dmaStreamSetMemory0(PDmaTx, Buf);
    dmaStreamSetMode(PDmaTx, SAI_DMATX_MONO_MODE);
    dmaStreamSetTransactionSize(PDmaTx, Sz16);
    dmaStreamEnable(PDmaTx);
    EnableSAI(); // Start tx
}

bool max98357_t::IsTransmitting() {
    return (dmaStreamGetTransactionSize(PDmaTx) != 0);
}

void max98357_t::Stop() {
    dmaStreamDisable(PDmaTx);
    DisableSAI();
    Clk.DisablePllSai2();
}


// LRCLK ONLY supports 8kHz, 16kHz, 32kHz, 44.1kHz, 48kHz, 88.2kHz and 96kHz
uint8_t max98357_t::SetupSampleRate(uint32_t Fs) {
    Stop();
//    PrintfI("Fs: %u\r", Fs);
    AU_SAI_A->CR1 &= ~SAI_xCR1_MCKDIV; // Clear MCK divider
    switch(Fs) {
        case 8000:
            // fSAI = 4MHz * 43 / 7 = 24.571429MHz; fSAI / (MCLK_div=12) / 256 = 8kHz
            Clk.SetupPllSai2(43, 2, 7);
            AU_SAI_A->CR1 |= 6UL << 20; // MCLKDIV=3 => div=6
            break;

        case 16000:
            // fSAI = 4MHz * 43 / 7 = 24.571429MHz; fSAI / (MCLK_div=6) / 256 = 16kHz
            Clk.SetupPllSai2(43, 2, 7);
            AU_SAI_A->CR1 |= 3UL << 20; // MCLKDIV=3 => div=6
            break;

        case 32000:
            // fSAI = 4MHz * 86 / 7 = 24.571429MHz; fSAI / (MCLK_div=6) / 256 = 32kHz
            Clk.SetupPllSai2(86, 2, 7);
            AU_SAI_A->CR1 |= 3UL << 20; // MCLKDIV=3 => div=6
            break;

        case 44100:
            // fSAI = 4MHz * 48 / 17 = 11.29411765MHz; fSAI / (MCLK_div=1) / 256 = 44.118kHz
            Clk.SetupPllSai2(48, 2, 17);
            AU_SAI_A->CR1 |= 0UL << 20; // MCLKDIV=0 => div=1
            break;

        case 48000:
            // fSAI = 4MHz * 43 / 7 = 24.571429MHz; fSAI / (MCLK_div=2) / 256 = 48kHz
            Clk.SetupPllSai2(43, 2, 7);
            AU_SAI_A->CR1 |= 1UL << 20; // MCLKDIV=1 => div=2
            break;

        case 96000:
            // fSAI = 4MHz * 43 / 7 = 24.571429MHz; fSAI / (MCLK_div=1) / 256 = 96kHz
            Clk.SetupPllSai2(43, 2, 7);
            AU_SAI_A->CR1 |= 0UL << 20; // MCLKDIV=0 => div=1
            break;

        default: return retvBadValue; break;
    } // switch
    if(Clk.EnablePllSai2() == retvOk) return retvOk;
    else {
        PrintfI("SAI1 fail\r");
        return retvFail;
    }
}

#if 1 // ============================== IRQ ====================================
extern "C"
OSAL_IRQ_HANDLER(SAI_IRQ_HANDLER) {
    OSAL_IRQ_PROLOGUE();
    if(AU_SAI_A->SR & SAI_xSR_FREQ) {
        AU_SAI_A->DR = 0x0001;
        AU_SAI_A->DR = 0x0003;
    }
    OSAL_IRQ_EPILOGUE();
}
#endif
