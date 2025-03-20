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

#define PWM_MOVEMENT_FACTOR		308 // pulses per mm, TODO Figure out if this is true
#define MIN_LOCATION_VALUE		0
#define MAX_LOCATION_VALUE		150 // Can only be twice as far as the middle :)
#define RESET_LOCATION_VALUE	0 // location in mm of where we are (meaning we can only move 10cm in either direction)


//extern struct tc_module tc_instance;
extern volatile uint16_t currLocationY;
extern volatile uint16_t moveDestinationY;

void tc_callback_to_change_duty_cycle(
	struct tc_module *const module_inst);
int initMotor(void);
int mot_move_to_loc(uint16_t location);
int mot_get_dir(void);
int mot_en_get(void);
int mot_moveLoc_get(void);
int mot_currPos_get(void);

#endif /* MOTORLIB_H_ */