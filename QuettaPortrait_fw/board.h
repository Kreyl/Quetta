/*
 * board.h
 *
 *  Created on: 12 сент. 2015 г.
 *      Author: Kreyl
 */

#pragma once

#include <inttypes.h>

// ==== General ====
#define BOARD_NAME          "Quetta2"
#define APP_NAME            "PortraitPass"

// MCU type as defined in the ST header.
#define STM32F205xx

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ 12000000

#define SYS_TIM_CLK     (Clk.APB1FreqHz) // OS timer settings
#define I2C_REQUIRED    FALSE
#define ADC_REQUIRED    FALSE
#define SIMPLESENSORS_ENABLED   FALSE

#if 1 // ========================== GPIO =======================================
// UART
#define UART_GPIO       GPIOA
#define UART_TX_PIN     2
#define UART_RX_PIN     3
#define UART_AF         AF7 // for USART2 @ GPIOA

// Battery measuremrnt
#define BAT_MEAS_ADC    GPIOC, 5

// LEDs
#define LED_PIN         GPIOB, 9, omPushPull
// WS2812
#define LEDWS_PIN       GPIOA, 7, omPushPull, pudNone, AF5, psHigh
#define LEDWS_SPI       SPI1

// Sound
#define VS_GPIO         GPIOB
#define VS_XCS          3
#define VS_XDCS         8
#define VS_RST          4
#define VS_DREQ         2
#define VS_XCLK         13
#define VS_SO           14
#define VS_SI           15
// Amplifier
#define VS_AMPF_EXISTS  TRUE
#define VS_AMPF_GPIO    GPIOA
#define VS_AMPF_PIN     15
#define VS_SPI          SPI2
#define VS_AF           AF5

#endif // GPIO

#if I2C_REQUIRED // ====================== I2C =================================
#define I2C1_ENABLED     TRUE
#define I2C_PIN       { GPIOA, 9, 10, I2C1_AF, I2C1_BAUDRATE, I2C1_DMA_TX, I2C1_DMA_RX}
#endif

#if 1 // ========================== USART ======================================
#define UART            USART2
#define UART_TX_REG     UART->DR
#define UART_RX_REG     UART->DR
#endif

#if ADC_REQUIRED // ======================= Inner ADC ==========================
// Clock divider: clock is generated from the APB2
#define ADC_CLK_DIVIDER		adcDiv4

// ADC channels
#define ADC_CHNL_A 	        2
#define ADC_CHNL_B 	        3
#define ADC_CHNL_C 	        4
#define ADC_CHNL_D 	        10
#define ADC_CHNL_E 	        11
#define ADC_CHNL_F 	        12
#define ADC_CHNL_G 	        13
#define ADC_CHNL_H 	        14

#define ADC_CHNL_BATTERY    15

#define ADC_CHANNELS        { ADC_CHNL_A, ADC_CHNL_B, ADC_CHNL_C, ADC_CHNL_D, ADC_CHNL_E, ADC_CHNL_F, ADC_CHNL_G, ADC_CHNL_H, ADC_CHNL_BATTERY, ADC_CHNL_VREFINT }
#define ADC_CHANNEL_CNT     10   // Do not use countof(AdcChannels) as preprocessor does not know what is countof => cannot check
#define ADC_SAMPLE_TIME     ast84Cycles
#define ADC_SAMPLE_CNT      1   // How many times to measure every channel

#define ADC_MAX_SEQ_LEN     16  // 1...16; Const, see ref man
#define ADC_SEQ_LEN         (ADC_SAMPLE_CNT * ADC_CHANNEL_CNT)
#if (ADC_SEQ_LEN > ADC_MAX_SEQ_LEN) || (ADC_SEQ_LEN == 0)
#error "Wrong ADC channel count and sample count"
#endif
#endif

#if 1 // =========================== DMA =======================================
#define STM32_DMA_REQUIRED  TRUE
// ==== Uart ====
// Remap is made automatically if required
#define UART_DMA_TX     STM32_DMA1_STREAM6
#define UART_DMA_RX     STM32_DMA1_STREAM5
#define UART_DMA_CHNL   4

// SDIO
#define STM32_SDC_SDIO_DMA_STREAM   STM32_DMA_STREAM_ID(2, 3)

#define LEDWS_DMA       STM32_DMA2_STREAM5  // SPI1 TX
#define LEDWS_DMA_CHNL  3                   // DMA2 Stream5 ch3

#if ADC_REQUIRED
#define ADC_DMA         STM32_DMA2_STREAM0
#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* DMA2 Stream0 Channel0 */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */
#endif // ADC

// Sound
#define VS_DMA          STM32_DMA1_STREAM4
#define VS_DMA_CHNL     0
#define VS_DMA_MODE     STM32_DMA_CR_CHSEL(VS_DMA_CHNL) | \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_BYTE | \
                        STM32_DMA_CR_PSIZE_BYTE | \
                        STM32_DMA_CR_DIR_M2P |    /* Direction is memory to peripheral */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */


#endif // DMA

#if 1 // =============================== SDIO ==================================
#define STM32_PLL48CLK      48000000
#define STM32_SDIO_DIV_HS   2   // Work clock:    48MHz / (2+2)   = 12MHz
#define STM32_SDIO_DIV_LS   120 // Initial clock: 48MHz / (120+2) = 393kHz, less than 400
#endif
