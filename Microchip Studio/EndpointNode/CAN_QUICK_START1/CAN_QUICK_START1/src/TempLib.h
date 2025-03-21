/*
 * TempLib.h
 *
 * Created: 4/25/2023 6:41:24 PM
 *  Author: Tyler
 */ 


#ifndef TEMPLIB_H_
#define TEMPLIB_H_
#include "Prf_endpoint.h"

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>

extern uint32_t TEMPOFFSETVAL;
extern int32_t tsens_result;

extern struct tsens_module tsens_instance;
extern volatile bool tsens_read_done;
// functions
int initTSENS(void);
int tsens_get_temp(void);
int get_tsens_offset(void);
int set_tsens_offset(uint32_t offset);

#if SYSTEM_NODE_TYPE == 4
	#pragma message ( "Tsens" )
dev_cap_structure cap_callback_ref = {
	.status_cb_get = &tsens_get_temp,
	.status_cb_get_alt = &get_tsens_offset,
	.write_cb_set = &set_tsens_offset,
	.write_cb_toggle = &def_cb_nodef_void,
	
};
#endif

#endif /* TEMPLIB_H_ */