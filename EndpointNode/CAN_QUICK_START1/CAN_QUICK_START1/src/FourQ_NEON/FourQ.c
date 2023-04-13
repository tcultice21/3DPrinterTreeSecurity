/***********************************************************************************
* FourQ: 4-dimensional decomposition on a Q-curve with CM in twisted Edwards form
*
*    Copyright (c) Microsoft Corporation. All rights reserved.
*
*
* Abstract: FourQ's curve parameters
*
* This code is based on the paper "FourQ: four-dimensional decompositions on a 
* Q-curve over the Mersenne prime" by Craig Costello and Patrick Longa, in Advances 
* in Cryptology - ASIACRYPT, 2015.
* Preprint available at http://eprint.iacr.org/2015/565.
************************************************************************************/  

#include "FourQ_internal.h"


// Encoding of field elements, elements over Z_r and elements over GF(p^2):
// -----------------------------------------------------------------------
// Elements over GF(p) and Z_r are encoded with the least significant octet (and digit) located
// in the leftmost position (i.e., little endian format). Elements (a+b*i) over GF(p^2), where
// a and b are defined over GF(p), are encoded as {b, a}, with b in the least significant position.


CurveStruct curve4Q = {
    256, 246,   // 2x targeted security level, and bitlength of the prime order subgroup
    // Prime p = 2^127-1
    { 0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF },                                                
    // Parameter "a" in GF(p^2)
    { 0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x0000000000000000, 0x0000000000000000 },
    // Parameter "d" in GF(p^2)
    { 0x0000000000000142, 0x00000000000000E4, 0xB3821488F1FC0C8D, 0x5E472F846657E0FC },
    // Order of the group
    { 0x2FB2540EC7768CE7, 0xDFBD004DFE0F7999, 0xF05397829CBC14E5, 0x0029CBC14E5E0A72 },
    // x(generator) in GF(p^2)
    { 0x286592AD7B3833AA, 0x1A3472237C2FB305, 0x96869FB360AC77F6, 0x1E1F553F2878AA9C },    
    // y(generator) in GF(p^2)
    { 0xB924A2462BCBB287, 0x0E3FEE9BA120785A, 0x49A7C344844C8B5C, 0x6E1C4AF8630E0242 }, 
    // co-factor
    392
};