/*
 * cmd_uart.h
 *
 *  Created on: 15.04.2013
 *      Author: kreyl
 */

#ifndef UART_H__
#define UART_H__

#include "kl_lib.h"
//#include <cstring>
#include "shell.h"
#include "board.h"

#define UART_USE_TXE_IRQ    FALSE

#define UART_CMD_BUF_SZ     54 // payload bytes

// ==== Base class ====
class BaseUart_t {
protected:
    // Params
    const uint32_t Baudrate;
    USART_TypeDef* Uart;
    const GPIO_TypeDef *PGpioTx;
    const uint16_t PinTx;
    const GPIO_TypeDef *PGpioRx;
    const uint16_t PinRx;
    // DMA
    const uint32_t DmaTxID, DmaRxID;
    const uint32_t DmaModeTx, DmaModeRx;
    uartClk_t ClkSrc;
    // Other
    const stm32_dma_stream_t *PDmaTx = nullptr;
    const stm32_dma_stream_t *PDmaRx = nullptr;
    char TXBuf[UART_TXBUF_SZ];
    volatile char *PRead, *PWrite;
    volatile bool IDmaIsIdle;
    volatile uint32_t IFullSlotsCount, ITransSize;
    void ISendViaDMA();
    int32_t RIndx;
    uint8_t IRxBuf[UART_RXBUF_SZ];
    uint8_t IPutByte(uint8_t b);
    uint8_t IPutByteNow(uint8_t b);
    void IStartTransmissionIfNotYet();
    // ==== Constructor ====
    BaseUart_t(
            uint32_t ABaudrate, USART_TypeDef* AUart,
            GPIO_TypeDef *APGpioTx, uint16_t APinTx,
            GPIO_TypeDef *APGpioRx, uint16_t APinRx,
            uint32_t ADmaTxID, uint32_t ADmaRxID,
            uint32_t ADmaModeTx, uint32_t ADmaModeRx, uartClk_t AClkSrc
    ) : Baudrate(ABaudrate), Uart(AUart),
            PGpioTx(APGpioTx), PinTx(APinTx), PGpioRx(APGpioRx), PinRx(APinRx),
            DmaTxID(ADmaTxID), DmaRxID(ADmaRxID),
            DmaModeTx(ADmaModeTx), DmaModeRx(ADmaModeRx)
#if defined STM32F072xB || defined STM32L4XX || defined STM32F7XX
        , ClkSrc(AClkSrc)
#endif
    , PRead(TXBuf), PWrite(TXBuf), IDmaIsIdle(true), IFullSlotsCount(0), ITransSize(0)
    , RIndx(0)
    {}
public:
    void Init();
    void Shutdown();
    void OnClkChange();
    // Enable/Disable
    void EnableTx()  { Uart->CR1 |= USART_CR1_TE; }
    void DisableTx() { Uart->CR1 &= ~USART_CR1_TE; }
    void EnableRx()  { Uart->CR1 |= USART_CR1_RE; }
    void DisableRx() { Uart->CR1 &= ~USART_CR1_RE; }
    void FlushTx() { while(!IDmaIsIdle) chThdSleepMilliseconds(1); }  // wait DMA
    void FlushRx();
    void EnableTCIrq(const uint32_t Priority, ftVoidVoid ACallback);
    void SetReplyEndChar(char c);
    // Inner use
    void IRQDmaTxHandler();
    uint8_t GetByte(uint8_t *b);
    virtual void OnUartIrqI(uint32_t flags) = 0;
};

class CmdUart_t : public BaseUart_t, public PrintfHelper_t, public Shell_t {
protected:
    uint8_t IPutChar(char c) { return IPutByte(c);  }
    void IStartTransmissionIfNotYet() { BaseUart_t::IStartTransmissionIfNotYet(); }
public:
    CmdUart_t(
            uint32_t ABaudrate, USART_TypeDef* AUart,
            GPIO_TypeDef *APGpioTx, uint16_t APinTx,
            GPIO_TypeDef *APGpioRx, uint16_t APinRx,
            uint32_t ADmaTxID, uint32_t ADmaRxID,
            uint32_t ADmaModeTx, uint32_t ADmaModeRx, uartClk_t AClkSrc
    ) : BaseUart_t(ABaudrate, AUart, APGpioTx, APinTx, APGpioRx, APinRx, ADmaTxID, ADmaRxID, ADmaModeTx, ADmaModeRx, AClkSrc) {}

    void Print(const char *format, ...) {
        va_list args;
        va_start(args, format);
        IVsPrintf(format, args);
        va_end(args);
    }

    uint8_t ReceiveBinaryToBuf(uint8_t *ptr, uint32_t Len, uint32_t Timeout_ms);
    uint8_t TransmitBinaryFromBuf(uint8_t *ptr, uint32_t Len, uint32_t Timeout_ms);
    uint8_t GetRcvdCmd();
    void OnUartIrqI(uint32_t flags);
};

class CmdUart485_t : public CmdUart_t {
private:
    GPIO_TypeDef *PGpioDE;
    uint16_t PinDE;
    AlterFunc_t AltFuncDE;
public:
    void Init() {
        CmdUart_t::Init();
        PinSetupAlterFunc(PGpioDE, PinDE, omPushPull, pudNone, AltFuncDE);
        Uart->CR1 &= ~USART_CR1_UE;   // Disable USART
        Uart->CR3 |= USART_CR3_DEM;   // Enable DriverEnable signal
        Uart->CR1 |= USART_CR1_UE;    // Enable USART
    }
    CmdUart485_t(
            uint32_t ABaudrate, USART_TypeDef* AUart,
            GPIO_TypeDef *APGpioTx, uint16_t APinTx,
            GPIO_TypeDef *APGpioRx, uint16_t APinRx,
            uint32_t ADmaTxID, uint32_t ADmaRxID,
            uint32_t ADmaModeTx, uint32_t ADmaModeRx, uartClk_t AClkSrc,
            GPIO_TypeDef *APGPIO, uint16_t APin, AlterFunc_t AAf
    ) : CmdUart_t(ABaudrate, AUart, APGpioTx, APinTx, APGpioRx, APinRx, ADmaTxID, ADmaRxID, ADmaModeTx, ADmaModeRx, AClkSrc),
            PGpioDE(APGPIO), PinDE(APin), AltFuncDE(AAf) {}
};

class HostUart485_t : private CmdUart485_t {
private:
    thread_reference_t ThdRef = nullptr;
    uint8_t TryParseRxBuff();
public:
    void Init() { CmdUart485_t::Init(); }
    HostUart485_t(
            uint32_t ABaudrate, USART_TypeDef* AUart,
            GPIO_TypeDef *APGpioTx, uint16_t APinTx,
            GPIO_TypeDef *APGpioRx, uint16_t APinRx,
            uint32_t ADmaTxID, uint32_t ADmaRxID,
            uint32_t ADmaModeTx, uint32_t ADmaModeRx, uartClk_t AClkSrc,
            GPIO_TypeDef *APGPIO, uint16_t APin, AlterFunc_t AAf
            ) :
        CmdUart485_t(ABaudrate, AUart, APGpioTx, APinTx, APGpioRx, APinRx, ADmaTxID, ADmaRxID, ADmaModeTx, ADmaModeRx, AClkSrc, APGPIO, APin, AAf) {}
    uint8_t SendCmd(uint32_t Timeout_ms, const char* ACmd, uint32_t Addr, const char *format = nullptr, ...);
    uint8_t SendCmdAndTransmitBuf(uint32_t Timeout_ms, uint8_t *PBuf, uint32_t Len, const char* ACmd, uint32_t Addr, const char *format = nullptr, ...);
    uint8_t SendCmdAndReceiveBuf(uint32_t Timeout_ms, uint8_t *PBuf, uint32_t Len, const char* ACmd, uint32_t Addr, const char *format = nullptr, ...);
    Cmd_t &Reply = Cmd;
    void OnUartIrqI(uint32_t flags);
};

//class JsonUart_t : public BaseUart_t, public PrintfHelper_t, public ShellJson_t {
//private:
//    uint8_t IPutChar(char c) { return IPutByte(c);  }
//    void IStartTransmissionIfNotYet() { BaseUart_t::IStartTransmissionIfNotYet(); }
//public:
//    JsonUart_t(
//            uint32_t ABaudrate, USART_TypeDef* AUart,
//            GPIO_TypeDef *APGpioTx, uint16_t APinTx,
//            GPIO_TypeDef *APGpioRx, uint16_t APinRx,
//            uint32_t ADmaTxID, uint32_t ADmaRxID,
//            uint32_t ADmaModeTx, uint32_t ADmaModeRx, uartClk_t AClkSrc
//            ) : BaseUart_t(ABaudrate, AUart, APGpioTx, APinTx, APGpioRx, APinRx, ADmaTxID, ADmaRxID, ADmaModeTx, ADmaModeRx, AClkSrc) {}
//    void Print(const char *format, ...) {
//        va_list args;
//        va_start(args, format);
//        IVsPrintf(format, args);
//        va_end(args);
//    }
//    uint8_t ReceiveBinaryToBuf(uint8_t *ptr, uint32_t Len, uint32_t Timeout_ms);
//    uint8_t TryParseRxBuff() {
//        uint8_t b;
//        while(GetByte(&b) == retvOk) {
//            if(Cmd.PutChar(b) == pdrNewCmd) return retvOk;
//        } // while get byte
//        return retvFail;
//    }
//    void OnUartIrqI(uint32_t flags);
//};

#define BYTE_UART_EN    FALSE
#if BYTE_UART_EN
class ByteUart_t : public BaseUart_t, public ByteShell_t {
//private:
//    uint8_t IPutChar(char c) { return IPutByte(c);  }
//    void IStartTransmissionIfNotYet() { BaseUart_t::IStartTransmissionIfNotYet(); }
public:
    ByteUart_t(const UartParams_t *APParams) : BaseUart_t(APParams) {}
    void Init(uint32_t ABaudrate);
    uint8_t IPutChar(char c) { return IPutByte(c); }
    void IStartTransmissionIfNotYet() { BaseUart_t::IStartTransmissionIfNotYet(); }
    void IRxTask();
};
#endif

#endif // UART_H__
