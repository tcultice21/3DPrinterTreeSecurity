/*
 * FanLib.h
 *
 * Created: 4/26/2023 12:52:15 AM
 *  Author: Marshmallow
 */ 

#ifndef FANLIB_H_
#define FANLIB_H_

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>

void configure_port_pins(void);
int testCode(void);
int initFan(void);


#endif /* FANLIB_H_ */