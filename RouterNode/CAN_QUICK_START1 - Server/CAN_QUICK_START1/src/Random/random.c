/***********************************************************************************
* FourQlib: a high-performance crypto library based on the elliptic curve FourQ
*
*    Copyright (c) Microsoft Corporation. All rights reserved.
*
* Abstract: pseudo-random function
************************************************************************************/ 

#include "random.h"
#include <stdlib.h>
#include <stdbool.h>
#if defined(__WINDOWS__)
    #include <windows.h>
    #include <bcrypt.h>
    #define RTL_GENRANDOM "SystemFunction036"
    NTSTATUS last_bcrypt_error = 0;
#elif defined(__LINUX__)
    #include <unistd.h>
    #include <fcntl.h>
    static int lock = -1;
#endif


static __inline void delay(unsigned int count)
{
    while (count--) {}
}


int random_bytes(unsigned char* random_array, unsigned int nbytes)
{ // Generation of "nbytes" of random values
	for(int i = 0; i < nbytes; i++) {
		random_array[i] = rand();
	}
    return true;
}