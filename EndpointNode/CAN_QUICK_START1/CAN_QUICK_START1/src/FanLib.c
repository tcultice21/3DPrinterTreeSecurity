/*
 * CFile1.c
 *
 * Created: 4/26/2023 12:50:20 AM
 *  Author: Marshmallow
 */ 
#include "FanLib.h"


int pin_state = 0;

void configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	/*config_port_pin.direction  = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(BUTTON_0_PIN, &config_port_pin);*/
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(FAN_ENABLE_PORT, &config_port_pin);
}

int initFan(void) {
	configure_port_pins();
	return 0;
}

int toggle_fan(void) {
	pin_state = !pin_state;
	port_pin_set_output_level(FAN_ENABLE_PORT, pin_state);
}



int fan_on(void) {
	pin_state = 1;
	port_pin_set_output_level(FAN_ENABLE_PORT, pin_state);
}



int fan_off(void) {
	pin_state = 0;
	port_pin_set_output_level(FAN_ENABLE_PORT, pin_state);
}



int set_fan_state(int i) {
	pin_state = i & 1;
	port_pin_set_output_level(FAN_ENABLE_PORT, pin_state);
}

int get_fan_state(void) {
	return pin_state;
}

int testCode(void) {
	while (1) {
		bool pin_state = 1;
		port_pin_set_output_level(FAN_ENABLE_PORT, pin_state);
		for(int i = 0; i < 10000000; i++);
		for(int i = 0; i < 10000000; i++);
		for(int i = 0; i < 10000000; i++);
		port_pin_set_output_level(FAN_ENABLE_PORT, !pin_state);
		for(int i = 0; i < 10000000; i++);
		for(int i = 0; i < 10000000; i++);
		for(int i = 0; i < 10000000; i++);
	};
	return 0;
}