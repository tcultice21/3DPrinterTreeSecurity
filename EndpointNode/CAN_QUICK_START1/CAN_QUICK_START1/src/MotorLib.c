/*
 * MotorLib.c
 *
 * Created: 4/26/2023 1:15:25 AM
 *  Author: Marshmallow
 */ 

//! [callback_funcs]
#include "MotorLib.h"

void configure_tc(void);
void configure_tc_callbacks(void);
void tc_callback_to_change_duty_cycle(
struct tc_module *const module_inst);

struct tc_module tc_instance;

void configure_tc(void);
void configure_tc_callbacks(void);
void tc_callback_to_change_duty_cycle(
struct tc_module *const module_inst);

#define PWM_MOVEMENT_FACTOR		400 // pulses per mm, TODO Figure out if this is true
#define RESET_LOCATION_VALUE	1000 // location in mm of where we are (meaning we can only move 10cm in either direction)
#define MIN_LOCATION_VALUE		0
#define MAX_LOCATION_VALUE		RESET_LOCATION_VALUE*2 // Can only be twice as far as the middle :)

/////////////////////////////////////////
// Official Joseph Certificate for     //
// 5mm Precision                       //
// Signature: Joseph Clark             //
// Notorized by: Tyler Cultice		   //
/////////////////////////////////////////

enum {
	MOTOR_STATE_IDLE = 0,
	MOTOR_STATE_MOVING = 1,
	MOTOR_STATE_DISABLED = 2
} motor_state = MOTOR_STATE_DISABLED;


//! [module_inst]
struct tc_module tc_instance;
//! [module_inst]

volatile uint16_t moveDestinationY = 0;
volatile uint16_t currLocationY = 1000;
static int mot_dir = 0;
static int mot_enable = 0;

// Motor EN/DIR pin enables/disables/configures
void configure_motor_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(MOTOR_DIR_PIN, &config_port_pin); // PB05 - Arduino Header = D8
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(MOTOR_EN_PIN, &config_port_pin); // PB04 - Arduino Header = D7
}

// Motor Enable Functions
void mot_en_enable(void) {
	mot_enable = 1;
	port_pin_set_output_level(MOTOR_EN_PIN, mot_enable);
}

// You USUALLY don't want to be doing what is below this!!! Snapmaker wants them "always on"
void mot_en_disable(void) {
	mot_enable = 0;
	port_pin_set_output_level(MOTOR_EN_PIN, mot_enable);
}

int mot_en_get(void) {
	return mot_enable;
}

// Motor Direction Functions
int mot_dir_toggle(void) {
	mot_dir = !mot_dir;
	port_pin_set_output_level(MOTOR_DIR_PIN, mot_dir);
	return mot_dir;
}

void mot_dir_set(int dirVal) {
	mot_dir = dirVal;
	port_pin_set_output_level(MOTOR_DIR_PIN, mot_dir);
}

int mot_dir_get(void) {
	return mot_dir;
}

//! PWM Helper Functions

//! [setup]
void configure_tc(void)
{
	//! [setup_config]
	struct tc_config config_tc;
	//! [setup_config]
	//! [setup_config_defaults]
	tc_get_config_defaults(&config_tc);
	//! [setup_config_defaults]

	//! [setup_change_config]
	config_tc.counter_size    = TC_COUNTER_SIZE_16BIT;
	config_tc.wave_generation = TC_WAVE_GENERATION_NORMAL_PWM;
	config_tc.counter_16_bit.compare_capture_channel[0] = 0xFFFF;
	//! [setup_change_config]

	//! [setup_change_config_pwm]
	config_tc.pwm_channel[0].enabled = true;
	config_tc.pwm_channel[0].pin_out = PWM_OUT_PIN;
	config_tc.pwm_channel[0].pin_mux = PWM_OUT_MUX;
	//! [setup_change_config_pwm]

	//! [setup_set_config]
	tc_init(&tc_instance, PWM_MODULE, &config_tc);
	//! [setup_set_config]

	//! [setup_enable]
	//tc_enable(&tc_instance); <-- We won't do this until we're ready
	//! [setup_enable]
}

//! Motor direct actions (probably not for user space)
void motor_shut_off(void) {
	tc_stop_counter(&tc_instance);
	motor_state = MOTOR_STATE_IDLE;
	//mot_enable = 1; // TODO: Double check what's enable but we shouldn't be turning this off according to snapmaker
}

void motor_turn_on(void) {
	tc_start_counter(&tc_instance);
	motor_state = MOTOR_STATE_MOVING;
	//mot_enable = 0; // Should already be this
}

//! [callback_funcs]
void tc_callback_to_change_duty_cycle(
struct tc_module *const module_inst)
{
	static uint16_t i = 0;
	if(i > PWM_MOVEMENT_FACTOR) {
		// We have moved 1mm
		currLocationY += (mot_dir*2-1);
		if (mot_dir == 1 && currLocationY >= moveDestinationY) {
			// Stop the TC PWM
			motor_shut_off();
		}
		else if (mot_dir == 0 && currLocationY <= moveDestinationY) {
			// Stop the TC PWM
			motor_shut_off();
		}
		i = 0;
	}
	else {
		i++;
	}
	//i += 16;
	//tc_set_compare_value(module_inst, TC_COMPARE_CAPTURE_CHANNEL_0, i + 1);
}

void configure_tc_callbacks(void)
{
	//! [setup_register_callback]
	tc_register_callback(
	&tc_instance,
	tc_callback_to_change_duty_cycle,
	TC_CALLBACK_CC_CHANNEL0);
	//! [setup_register_callback]

	//! [setup_enable_callback]
	tc_enable_callback(&tc_instance, TC_CALLBACK_CC_CHANNEL0);
	//! [setup_enable_callback]
}
//! [setup]

// User space functions

// Init Functions
// This will NOT make the motor move, just start it up
int initMotor() {
	configure_tc();
	configure_tc_callbacks();
	
	system_interrupt_enable_global();
	
	// Assume user has brought the device to the middle (no endstops for us yet)
	currLocationY = RESET_LOCATION_VALUE;
	moveDestinationY = RESET_LOCATION_VALUE;
	mot_dir_set(0);
	mot_en_disable(); // TODO double check this is init'ed to "shut off motor"
	motor_state = MOTOR_STATE_IDLE;
	
	return 0;
}

// Set move location
// return -2 means you never called init
// return 1 means you gave the same location as its already at
// return -1 means you have something out of range
int mot_move_to_loc(uint16_t location) {
	// Double check the motor is even alive
	if (motor_state == MOTOR_STATE_DISABLED) {
		return -2;
	}
	
	// Double check we aren't already there
	if (location == currLocationY) {
		return 1;
	}
	
	// Make sure the value is reasonable
	if (location < 0 || location > MAX_LOCATION_VALUE) {
		return -1;
	}
	
	moveDestinationY = location;
	
	// Set the direction
	if (location < currLocationY) {
		mot_dir_set(1); // Double check this is right!!!
	}
	else {
		mot_dir_set(0);
	}
	
	// Enable pin should already be set.
	
	
	// Re-enable the counter, meaning PWM is back online
	if(motor_state == MOTOR_STATE_IDLE) {
		motor_turn_on();
	}
}