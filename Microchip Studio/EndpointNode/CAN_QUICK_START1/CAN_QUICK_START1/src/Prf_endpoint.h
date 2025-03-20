/*
 * Prf_endpoint.h
 *
 * Created: 3/20/2025 4:47:58 PM
 *  Author: Marshmallow
 */ 


#ifndef PRF_ENDPOINT_H_
#define PRF_ENDPOINT_H_

#include<stdint.h>

#define CB_VAL_STATUS 0
#define CB_VAL_STATUS_ALT 1
#define CB_TEST 2

#define CB_VAL_WRITE 0
#define CB_VAL_TOGGLE 1

// #define EP_2A_MOTOR
// #define SYSTEM_ENDPOINT_TYPE
// #define SYSTEM_NODE_TYPE 2

typedef struct dev_cap_structure {
	int32_t (* status_cb_get)(void);
	int32_t (* status_cb_get_alt)(void);
	void (*write_cb_set)(int);
	void (*write_cb_toggle)();
} dev_cap_structure;

int def_cb_nodefine_ret(void);
void def_cb_nodef_void(void);


#endif /* PRF_ENDPOINT_H_ */