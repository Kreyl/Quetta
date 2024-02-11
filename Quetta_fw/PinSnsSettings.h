/*
 * SnsPins.h
 *
 *  Created on: 17 ���. 2015 �.
 *      Author: Kreyl
 */

/* ================ Documentation =================
 * There are several (may be 1) groups of sensors (say, buttons and USB connection).
 *
 */

#pragma once

#include "SimpleSensors.h"

#ifndef SIMPLESENSORS_ENABLED
#define SIMPLESENSORS_ENABLED   FALSE
#endif

#if SIMPLESENSORS_ENABLED
#define SNS_POLL_PERIOD_MS      306

// Handlers
extern void ProcessUsbPin(PinSnsState_t *PState, uint32_t Len);
extern void ProcessSnsPin(PinSnsState_t *PState, uint32_t Len);

const PinSns_t PinSns[] = {
        // Buttons
        {USB_DETECT_PIN, pudPullDown, ProcessUsbPin},
        {SNS_INPUT, pudPullDown, ProcessSnsPin},
};
#define PIN_SNS_CNT     countof(PinSns)

#endif  // if enabled
