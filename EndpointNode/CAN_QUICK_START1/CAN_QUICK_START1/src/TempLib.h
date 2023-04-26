/*
 * TempLib.h
 *
 * Created: 4/25/2023 6:41:24 PM
 *  Author: Marshmallow
 */ 


#ifndef TEMPLIB_H_
#define TEMPLIB_H_

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>

extern int32_t tsens_result;
extern struct tsens_module tsens_instance;
extern volatile bool tsens_read_done;

// Testing function
int initTSENS();
int tsens_get_temp();

#endif /* TEMPLIB_H_ */