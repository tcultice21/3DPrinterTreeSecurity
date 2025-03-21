/*
 * TempLib.c
 *
 * Created: 4/25/2023 6:40:16 PM
 *  Author: Tyler
 */ 

#include "TempLib.h"

//! [result]
int32_t tsens_result = 0;
//! [result]



//! [module_inst]
struct tsens_module tsens_instance;
//! [module_inst]

volatile bool tsens_read_done = false;
uint32_t TEMPOFFSETVAL = 0;

static void tsens_complete_callback(enum tsens_callback i)
{
	tsens_read_done = true;
}
//! [job_complete_callback]



//! [setup]
static void configure_tsens(void)
{
	//! [setup_config]
	struct tsens_config config_tsens;
	//! [setup_config]
	//! [setup_config_defaults]
	tsens_get_config_defaults(&config_tsens);
	//! [setup_config_defaults]

	

	//! [setup_set_config]
	tsens_init(&config_tsens);
	//! [setup_set_config]

	

	//! [setup_enable]
	tsens_enable();
	//! [setup_enable]
}



static void configure_tsens_callbacks(void)
{
	//! [setup_register_callback]
	tsens_register_callback(&tsens_instance,
	tsens_complete_callback, TSENS_CALLBACK_RESULT_READY);
	//! [setup_register_callback]
	//! [setup_enable_callback]
	tsens_enable_callback(TSENS_CALLBACK_RESULT_READY);
	//! [setup_enable_callback]
}
//! [setup]

int initTSENS(void) {
	configure_tsens();
	configure_tsens_callbacks();
	
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_TSENS);
	system_interrupt_enable_global();
	return 0;
}

int set_tsens_offset(uint32_t offset) {
	TEMPOFFSETVAL = offset;
	return 0;
}

int get_tsens_offset(void) {
	return TEMPOFFSETVAL;
}

int tsens_get_temp(void) {
	tsens_read_job(&tsens_instance, &tsens_result);

	while (tsens_read_done == false) {
	}
	tsens_read_done = false;
	tsens_read_job(&tsens_instance, &tsens_result);

	return (-TEMPOFFSETVAL-(int)tsens_result)/100;
}