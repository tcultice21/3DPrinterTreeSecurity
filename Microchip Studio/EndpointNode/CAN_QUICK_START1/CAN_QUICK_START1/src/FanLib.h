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

#define FAN_ENABLE_PORT PIN_PB04

extern int pin_state;

void configure_port_pins(void);
int testCode(void);
int initFan(void);

int fan_on(void);
int fan_off(void);
int set_fan_state(int i);
int toggle_fan(void);
int get_fan_state(void);

#endif /* FANLIB_H_ */