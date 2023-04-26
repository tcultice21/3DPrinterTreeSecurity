/*
 * MotorLib.h
 *
 * Created: 4/26/2023 1:15:44 AM
 *  Author: Marshmallow
 */ 


#ifndef MOTORLIB_H_
#define MOTORLIB_H_

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>

extern struct tc_module tc_instance;

void tc_callback_to_change_duty_cycle(
struct tc_module *const module_inst);


#endif /* MOTORLIB_H_ */