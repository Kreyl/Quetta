/*
 * FwUpdFrFilL476.h
 *
 *  Created on: 11 февр. 2023 г.
 *      Author: layst
 */

#ifndef KL_LIB_FWUPDFRFILL476_H_
#define KL_LIB_FWUPDFRFILL476_H_

#define FILENAME_PATTERN        "fw*.bin"

namespace FwUpdater {
    void PrintCurrBank();
    void CheckAndTryToUpdate();
}

#endif /* KL_LIB_FWUPDFRFILL476_H_ */
