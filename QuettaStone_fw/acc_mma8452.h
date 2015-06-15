/*
 * File:   acc_mma.h
 * Author: Kreyl
 *
 * Created on June 4, 2011, 11:56 AM
 */

#ifndef ACC_MMA_H
#define	ACC_MMA_H

#include "kl_lib.h"
#include "ch.h"
#include "hal.h"
#include "uart.h"

#define ACC_IRQ_GPIO            GPIOC
#define ACC_IRQ_PIN             3

#define ACC_MOTION_TRESHOLD     18      // 1...127. The threshold resolution is 0.063g/LSB.

#define ACC_I2C_ADDR            0x1C

// Registers addresses
#define ACC_REG_STATUS          0x00
#define ACC_INT_SRC             0x0C
#define ACC_REG_XYZ_DATA_CFG    0x0E
#define ACC_FF_MT_CFG           0x15
#define ACC_FF_MT_SRC           0x16
#define ACC_FF_MT_THS           0x17
#define ACC_FF_MT_COUNT         0x18
#define ACC_REG_CONTROL1        0x2A
#define ACC_REG_CONTROL2        0x2B
#define ACC_REG_CONTROL3        0x2C
#define ACC_REG_CONTROL4        0x2D
#define ACC_REG_CONTROL5        0x2E

//#define ACC_ACCELERATIONS_NEEDED

#ifdef ACC_ACCELERATIONS_NEEDED
struct Accelerations_t {
    uint8_t Status;     // Read to reset latched data
    int8_t xMSB, xLSB, yMSB, yLSB, zMSB, zLSB;
} PACKED;
#define ACCELERATIONS_SIZE     sizeof(Accelerations_t)
#endif

//#define ACC_IRQPIN_NEEDED

#ifdef ACC_IRQPIN_NEEDED
#if ACC_IRQ_PIN == 0
#define ACC_IRQ_HANDLER      EXTI0_IRQHandler
#elif ACC_IRQ_PIN == 1
#define ACC_IRQ_HANDLER     EXTI1_IRQHandler
#elif ACC_IRQ_PIN == 2
#define ACC_IRQ_HANDLER     EXTI2_IRQHandler
#elif ACC_IRQ_PIN == 3
#define ACC_IRQ_HANDLER     EXTI3_IRQHandler
#elif ACC_IRQ_PIN == 4
#define ACC_IRQ_HANDLER     EXTI4_IRQHandler
#endif
#endif

extern i2c_t i2c;

class Acc_t {
private:
    void IWriteReg(uint8_t AAddr, uint8_t AValue) {
        uint8_t RegAddr = AAddr, RegValue = AValue;
        if(i2c.CmdWriteWrite(ACC_I2C_ADDR, &RegAddr, 1, &RegValue, 1) != OK)
            Uart.Printf("\r!Acc WReg failure (%u %u)", AAddr, AValue);
    }
    void IClearIrq() { // Dummy read
        uint8_t RegAddr = ACC_FF_MT_SRC, Dummy;
        i2c.CmdWriteRead(ACC_I2C_ADDR, &RegAddr, 1, &Dummy, 1);
    }
public:
#ifdef ACC_ACCELERATIONS_NEEDED
    Accelerations_t Accelerations;
    uint32_t ThresholdTop, ThresholdBottom;
    void ReadAccelerations() {
        uint8_t RegAddr = ACC_REG_STATUS;
        i2c.CmdWriteRead(ACC_I2C_ADDR, &RegAddr, 1, (uint8_t*)&Accelerations, ACCELERATIONS_SIZE);
    }
#endif
    void Init();
    void Task();
#ifdef ACC_IRQPIN_NEEDED
    void IIrqHandler();
    PinIrq_t IIrqPin;
#endif
};

extern Acc_t Acc;

#endif	/* ACC_MMA_H */

