/*
 * aesSmall.h
 *
 * Created: 7/31/2023 12:15:31 PM
 *  Author: tcult
 */ 


#ifndef AESSMALL_H_
#define AESSMALL_H_

#include <stdint.h>
#include <string.h>
#include "aes.h"
#include "bearssl_block.h"

#define br_aes_small_BLOCK_SIZE   16

extern const unsigned char br_aes_S[];

typedef union {
	uint16_t u;
	unsigned char b[sizeof(uint16_t)];
} br_union_u16;

typedef union {
	uint32_t u;
	unsigned char b[sizeof(uint32_t)];
} br_union_u32;

typedef union {
	uint64_t u;
	unsigned char b[sizeof(uint64_t)];
} br_union_u64;

/*
 * AES block encryption with the 'aes_small' implementation (small, but
 * slow and not constant-time). This function encrypts a single block
 * "in place".
 */
void br_aes_small_encrypt(unsigned num_rounds,
	const uint32_t *skey, void *data);

/*
 * AES block decryption with the 'aes_small' implementation (small, but
 * slow and not constant-time). This function decrypts a single block
 * "in place".
 */
void br_aes_small_decrypt(unsigned num_rounds,
	const uint32_t *skey, void *data);

inline void aes256_encrypt(unsigned char* key, unsigned int size, unsigned char* input, unsigned char* output) {
	
}

inline void aes256_decrypt(unsigned char* key, unsigned int size, unsigned char* input, unsigned char* output) {
	
}


#endif /* AESSMALL_H_ */