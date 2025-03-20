/*
 * EPPeripherals.h
 *
 * Created: 4/28/2023 8:59:36 PM
 *  Author: Marshmallow
 */ 


#ifndef EPPERIPHERALS_H_
#define EPPERIPHERALS_H_

#include "CANLib.h"
#include "EncLib.h"
#include "TreeProtocol/node.h"

///////// Pick one based on your application //////////
#define TESTING

#if defined(TESTING)
	#include "MotorLib.h"
	#include "FanLib.h"
	#include "TempLib.h"
#elif SYSTEM_NODE_TYPE == NODE_TYPE_MOTOR
	#include "MotorLib.h"
#elif SYSTEM_NODE_TYPE == NODE_TYPE_FAN
	#include "FanLib.h"
#elif SYSTEM_NODE_TYPE == NODE_TYPE_TSENS
	#include "TempLib.h"
#endif



#endif /* EPPERIPHERALS_H_ */