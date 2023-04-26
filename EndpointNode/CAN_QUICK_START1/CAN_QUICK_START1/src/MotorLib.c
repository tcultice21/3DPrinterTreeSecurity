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

void tc_callback_to_change_duty_cycle(
struct tc_module *const module_inst)
{
	static uint16_t i = 0;

	i += 128;
	tc_set_compare_value(module_inst, TC_COMPARE_CAPTURE_CHANNEL_0, i + 1);
}

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
	tc_enable(&tc_instance);
	//! [setup_enable]
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

int initMotor() {
	configure_tc();
	configure_tc_callbacks();
	
	system_interrupt_enable_global();
	return 0;
}

//TODO: Add the I/O Pin needed (Dir, bc EN isn't needed)
// Change the callback to something more useful
// Give the ability to turn on and off the TC (look up how)