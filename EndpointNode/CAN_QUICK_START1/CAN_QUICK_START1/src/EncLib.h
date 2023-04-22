/*
 * EncLib.h
 *
 * Created: 4/18/2023 2:09:41 AM
 *  Author: Marshmallow
 */ 


#ifndef ENCLIB_H_
#define ENCLIB_H_

#include "CANLib.h"
#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>
#include "ASCON/api.h"
#include "ASCON/ascon.h"
#include "ASCON/crypto_aead.h"
#include "ASCON/constants.h"
#include "Enrollment.h"
#include "FourQ/FourQ_api.h"
#include "Photon/photon.h"
#define FOURQ_KEY_SIZE 32

struct ASCON_data {
	uint8_t session_key[ASCON_128_KEYBYTES];
	volatile uint8_t nonce[CRYPTO_NPUBBYTES];
};

uint8_t serverPUFResponse[16];

struct Crypto_Data {
	uint8_t secret_key[FOURQ_KEY_SIZE];
	uint8_t public_key[FOURQ_KEY_SIZE];
	uint8_t shared_secret[FOURQ_KEY_SIZE];
	uint8_t shared_hash[ASCON_128_KEYBYTES];
	struct ASCON_data ASCON_data;
	//uint8_t session_key[ASCON_128_KEYBYTES];
	//volatile uint8_t nonce[CRYPTO_NPUBBYTES];
} Crypto_Data;

extern struct Crypto_Data selfData;


#endif /* ENCLIB_H_ */