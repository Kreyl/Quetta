/*
 * evt_mask.h
 *
 *  Created on: Apr 12, 2013
 *      Author: g.kruglov
 */

#pragma once

// Event masks
#define EVT_UART_NEW_CMD        EVENT_MASK(1)

#define EVT_BUTTONS             EVENT_MASK(2)
#define EVT_PRESS_TIMEOUT       EVENT_MASK(3)

#define EVT_DOOR_OPEN_END       EVENT_MASK(4)
#define EVT_TOOMANYTRIES_END    EVENT_MASK(5)

#define EVT_LED_DONE            EVENT_MASK(14)

#define EVT_SAMPLING            EVENT_MASK(16)
#define EVT_ADC_DONE            EVENT_MASK(17)
#define EVT_PLAY_ENDS           EVENT_MASK(18)
