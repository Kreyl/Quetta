/*
 * Sequences.h
 *
 *  Created on: 09 ���. 2015 �.
 *      Author: Kreyl
 */

#ifndef SEQUENCES_H__
#define SEQUENCES_H__

#include "ChunkTypes.h"

const BaseChunk_t lsqStart[] = {
        {csSetup, 1},
        {csWait, 360},
        {csSetup, 0},
        {csWait, 360},
        {csGoto, 0},
};

const BaseChunk_t lsqUsbReady[] = {
        {csSetup, 1},
        {csEnd},
};

const BaseChunk_t lsqCmd[] = {
        {csSetup, 1},
        {csWait, 135},
        {csSetup, 0},
        {csEnd}
};

#endif //SEQUENCES_H__