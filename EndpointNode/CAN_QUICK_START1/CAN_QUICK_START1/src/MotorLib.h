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

//! [definition_pwm]
/** PWM module to use */
#define PWM_MODULE      EXT1_PWM_MODULE // PB12, PB13
/** PWM output pin */
#define PWM_OUT_PIN     EXT1_PWM_0_PIN // Arduino Header = D3
/** PWM output pinmux */
#define PWM_OUT_MUX     EXT1_PWM_0_MUX
//! [definition_pwm]

#define MOTOR_DIR_PIN	PIN_PB05 // Arduino Header = D8
#define MOTOR_EN_PIN	PIN_PB04 // Arduino Header = D7

//extern struct tc_module tc_instance;

void tc_callback_to_change_duty_cycle(
	struct tc_module *const module_inst);
int initMotor();
int mot_move_to_loc(uint16_t location);


#endif /* MOTORLIB_H_ */