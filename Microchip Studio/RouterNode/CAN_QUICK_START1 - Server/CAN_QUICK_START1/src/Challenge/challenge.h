#ifndef __CHALLENGE_H__
#define __CHALLENGE_H__
#include "stdint.h"

void pufResponse(uint8_t *res);
void pufResponseEnroll(uint8_t num_reads, uint8_t *res, uint8_t *ec);
void pufResponseCorrection(uint8_t *res, uint8_t *ec);
void printResponse(uint8_t *res);

#endif // __challenge_H__
