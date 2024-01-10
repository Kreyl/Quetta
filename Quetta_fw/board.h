/*
 * board.h
 *
 *  Created on: 10 01 2024
 *      Author: Kreyl
 */

#ifndef BOARD_H__
#define BOARD_H__

// ==== General ====
#define APP_NAME            "QuettaPortrait"

// MCU type as defined in the ST header.
#define STM32L476xx

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ         12000000

//  Periphery
#define I2C1_ENABLED            FALSE
#define I2C2_ENABLED            FALSE
#define I2C3_ENABLED            FALSE

#define ADC_REQUIRED            FALSE
#define SIMPLESENSORS_ENABLED   TRUE
#define STM32_DMA_REQUIRED      TRUE    // Leave this macro name for OS

#if 1 // ========================= Timers ======================================
// OS timer settings
#define STM32_ST_IRQ_PRIORITY   2
#define STM32_ST_USE_TIMER      5
#define SYS_TIM_CLK             (Clk.APB1FreqHz)    // Timer 5 is clocked by APB1

#endif

#if 1 // ========================== GPIO =======================================
// EXTI
#define INDIVIDUAL_EXTI_IRQ_REQUIRED    FALSE

// Debug pin
#define DBG_PIN         GPIOA, 8
#define DBG_HI()        PinSetHi(DBG_PIN)
#define DBG_LO()        PinSetLo(DBG_PIN)
#define DBG_TOGGLE()    PinToggle(DBG_PIN)

// WKUP pin
#define SNS_INPUT       GPIOA, 0 // WKUP1
// Charging
#define IS_CHARGING_PIN GPIOC, 15
// USB detect
#define USB_DETECT_PIN  GPIOA, 2 // WKUP4

// LEDs
#define LED_PIN         { GPIOB, 0, TIM3, 3, invNotInverted, omPushPull, 255 }

// Audio
#define AU_SAI          SAI1
#define AU_SAI_A        SAI1_Block_A
#define AU_SAI_RccEn()  RCC->APB2ENR |= RCC_APB2ENR_SAI1EN
#define AU_SAI_RccDis() RCC->APB2ENR &= ~RCC_APB2ENR_SAI1EN
#define AU_SDMODE       GPIOB, 8, omPushPull
#define AU_PWREN        GPIOC, 2, omPushPull
#define AU_LRCK         GPIOB, 9, omPushPull, pudNone, AF13
#define AU_SCLK         GPIOB, 10, omPushPull, pudNone, AF13
#define AU_SDIN         GPIOC, 3, omPushPull, pudNone, AF13 // MOSI; SAI1_A

// Acc
#define ACG_SPI         SPI2
#define ACG_PWR_PIN     GPIOB, 11
#define ACG_CS_PIN      GPIOB, 12
#define ACG_SCK_PIN     GPIOB, 13, omPushPull, pudNone, AF5
#define ACG_MISO_PIN    GPIOB, 14, omPushPull, pudNone, AF5
#define ACG_MOSI_PIN    GPIOB, 15, omPushPull, pudNone, AF5
#define ACG_IRQ_PIN     GPIOC, 13 // WKUP2

// ADC
#define ADC_BAT_EN      GPIOC, 4, omOpenDrain
#define ADC_BAT         GPIOC, 5

// SD
#define SD_PWR_PIN      GPIOC, 7
#define SD_AF           AF12
#define SD_DAT0         GPIOC,  8, omPushPull, pudPullUp, SD_AF
#define SD_DAT1         GPIOC,  9, omPushPull, pudPullUp, SD_AF
#define SD_DAT2         GPIOC, 10, omPushPull, pudPullUp, SD_AF
#define SD_DAT3         GPIOC, 11, omPushPull, pudPullUp, SD_AF
#define SD_CLK          GPIOC, 12, omPushPull, pudNone,   SD_AF
#define SD_CMD          GPIOD,  2, omPushPull, pudPullUp, SD_AF

// CMD UART
#define CMD_UART_GPIO   GPIOA
#define CMD_UART_TX_PIN 9
#define CMD_UART_RX_PIN 10

// USB
#define USB_DM          GPIOA, 11
#define USB_DP          GPIOA, 12
#define USB_AF          AF10

// Radio: SPI, PGpio, Sck, Miso, Mosi, Cs, Gdo0
#define CC_Setup0       SPI1, GPIOA, 5,6,7, GPIOA,15, GPIOA,4

// I2C
#define I2CA            i2c3
#define I2C3_GPIO       GPIOC
#define I2C3_SCL        0
#define I2C3_SDA        1
#define I2C_AF          AF4
// i2cclkPCLK1, i2cclkSYSCLK, i2cclkHSI
#define I2C_CLK_SRC     i2cclkHSI
#define I2C_BAUDRATE_HZ 400000
// I2C PullUp
#define I2C_PULLUP      GPIOC, 2, omPushPull

#endif // GPIO


#if 1 // =========================== DMA =======================================
#define ADC_DMA         STM32_DMA_STREAM_ID(1, 1)
#define CRC_DMA         STM32_DMA_STREAM_ID(1, 2)
#define ACG_DMA_RX      STM32_DMA_STREAM_ID(1, 4)
#define ACG_DMA_TX      STM32_DMA_STREAM_ID(1, 5)
#define SAI_DMA_A       STM32_DMA_STREAM_ID(2, 1)
#define STM32_SDC_SDMMC1_DMA_STREAM   STM32_DMA_STREAM_ID(2, 5)
#define UART_DMA_TX     STM32_DMA_STREAM_ID(2, 6)
#define UART_DMA_RX     STM32_DMA_STREAM_ID(2, 7)

// ==== Uart ====
// Remap is made automatically if required
#define UART_DMA_CHNL   2
#define UART_DMA_TX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_LOW | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_TCIE)
#define UART_DMA_RX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_MEDIUM | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC)

// ==== ACG ====
#define ACG_DMA_CHNL    1

// ==== SAI ====
#define SAI_DMA_CHNL    1

// ==== I2C ====
#define I2C_USE_DMA         FALSE
#if I2C1_ENABLED
#define I2C1_DMA_TX     STM32_DMA_STREAM_ID(1, 6)
#define I2C1_DMA_RX     STM32_DMA_STREAM_ID(1, 7)
#define I2C1_DMA_CHNL   3
#endif
#if I2C2_ENABLED
#define I2C2_DMA_TX     STM32_DMA_STREAM_ID(1, 4)
#define I2C2_DMA_RX     STM32_DMA_STREAM_ID(1, 5)
#define I2C2_DMA_CHNL   3
#endif
#if I2C3_ENABLED
#define I2C3_DMA_TX     STM32_DMA_STREAM_ID(1, 2)
#define I2C3_DMA_RX     STM32_DMA_STREAM_ID(1, 3)
#define I2C3_DMA_CHNL   3
#endif

#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* DMA1 Stream1 Channel0 */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */

#endif // DMA

#if 1 // ========================== USART ======================================
#define PRINTF_FLOAT_EN FALSE
#define UART_TXBUF_SZ   2048
#define UART_RXBUF_SZ   1024
#define CMD_BUF_SZ      1024

#define CMD_UART        USART1

#define CMD_UART_PARAMS \
    CMD_UART, CMD_UART_GPIO, CMD_UART_TX_PIN, CMD_UART_GPIO, CMD_UART_RX_PIN, \
    UART_DMA_TX, UART_DMA_RX, UART_DMA_TX_MODE(UART_DMA_CHNL), UART_DMA_RX_MODE(UART_DMA_CHNL), \
    uartclkHSI // Use independent clock

#endif

#endif //BOARD_H__
