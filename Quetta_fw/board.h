/*
 * board.h
 *
 *  Created on: 05 08 2018
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

// LEDs
#define LED_PIN        GPIOB, 0, omPushPull

// CMD UART
#define CMD_UART_GPIO   GPIOA
#define CMD_UART_TX_PIN 9
#define CMD_UART_RX_PIN 10

// USB
#define USB_DM          GPIOA, 11
#define USB_DP          GPIOA, 12
#define USB_AF          AF10


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
#define I2C_USE_DMA         FALSE
#define I2C3_DMA_TX         STM32_DMA_STREAM_ID(1, 2)
#define I2C3_DMA_RX         STM32_DMA_STREAM_ID(1, 3)
#define I2C3_DMA_CHNL       3

#define UART_DMA_RX         STM32_DMA_STREAM_ID(2, 7)
#define UART_DMA_TX         STM32_DMA_STREAM_ID(2, 6)
#define UART_DMA_CHNL       2

// Modes
#define UART_DMA_TX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_LOW | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_TCIE)
#define UART_DMA_RX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_MEDIUM | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC)

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
