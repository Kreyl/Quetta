/*
 * evt_mask.h
 *
 *  Created on: Apr 12, 2013
 *      Author: g.kruglov
 */

#pragma once

// Event masks
#define EVT_UART_NEW_CMD        EVENT_MASK(1)

#define EVT_LED_DONE            EVENT_MASK(4)

#define EVT_SAMPLING            EVENT_MASK(6)
#define EVT_ADC_DONE            EVENT_MASK(7)
#define EVT_PLAY_ENDS           EVENT_MASK(8)
