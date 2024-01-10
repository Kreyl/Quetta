/*
 * kl_crypto.h
 *
 *  Created on: 28 июн. 2023 г.
 *      Author: laurelindo
 */

#ifndef KL_LIB_KL_CRYPTO_H_
#define KL_LIB_KL_CRYPTO_H_

#include <inttypes.h>

namespace Sha256 {

// Context storage
struct Buff_t {
    uint32_t DataSz;
    uint32_t h[8];
    uint8_t LastChunk[64];
    uint8_t ChunkSz;
};

// Initialization, must be called before any further use
void Init(Buff_t *pBuf);

// Process block of data of arbitary length
void Update(Buff_t *pBuf, const void* pData, uint32_t Sz);

// Produces final hash values (digest) to be read
void Finalize(Buff_t *pBuf);

// Read digest into 32-byte binary array
void Read2BinArr(Buff_t *pBuf, uint8_t* Hash);

// Read digest into 64-char string as hex (without null-byte)
void Read2String(Buff_t *pBuf, char* S);

// Hash array and return 32-byte binary array
void Hash2BinArr(const void *pData, uint32_t Sz, uint8_t* Hash);


} // namespace

#endif /* KL_LIB_KL_CRYPTO_H_ */
