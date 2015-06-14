/*
 * SoundList.h
 *
 *  Created on: 14 θώνÿ 2015 γ.
 *      Author: Kreyl
 */

#ifndef SOUNDLIST_H_
#define SOUNDLIST_H_

#include "kl_sd.h"

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



#endif /* SOUNDLIST_H_ */
