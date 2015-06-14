/*
 * main.h
 *
 *  Created on: 30.03.2013
 *      Author: kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

// External Power Input
#define PWR_EXTERNAL_GPIO   GPIOA
#define PWR_EXTERNAL_PIN    9
static inline bool ExternalPwrOn() { return  PinIsSet(PWR_EXTERNAL_GPIO, PWR_EXTERNAL_PIN); }

// External sensors
class Sns_t {
private:
    bool IsHi() const { return PinIsSet(PGpioPort, PinNumber); }
public:
    GPIO_TypeDef *PGpioPort;
    uint16_t PinNumber;
    bool WasHi;
    void Init() const { PinSetupIn(PGpioPort, PinNumber, pudPullDown); }
    RiseFall_t CheckEdge() {
        if(!WasHi and IsHi()) {
            WasHi = true;
            return Rising;
        }
        else if(WasHi and !IsHi()) {
            WasHi = false;
            return Falling;
        }
        else return NoRiseNoFall;
    }
};

// ==== Sound files ====
#define SND_COUNT_MAX   100

struct Snd_t {
     char Filename[MAX_NAME_LEN];
     uint32_t ProbBottom, ProbTop;
};

struct SndList_t {
    Snd_t Phrases[SND_COUNT_MAX];
    int32_t Count;
    int32_t ProbSumm;
};

uint8_t ReadConfig();

#endif /* MAIN_H_ */
