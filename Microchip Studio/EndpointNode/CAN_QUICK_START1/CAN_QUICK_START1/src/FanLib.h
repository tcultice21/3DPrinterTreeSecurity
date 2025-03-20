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
#include "Prf_endpoint.h"

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

#if SYSTEM_NODE_TYPE == 3
	#pragma message ( "Fan" )
	dev_cap_structure cap_callback_ref = {
		.status_cb_get = &get_fan_state,
		.status_cb_get_alt = &def_cb_nodef_void,
		.write_cb_set = &set_fan_state,
		.write_cb_toggle = &toggle_fan,
		
	};
#endif

#endif /* FANLIB_H_ */