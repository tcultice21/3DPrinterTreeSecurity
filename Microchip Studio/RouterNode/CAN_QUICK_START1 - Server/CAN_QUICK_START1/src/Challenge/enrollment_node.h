/*
 * enrollment_node.h
 *
 *  Created on: Oct 27, 2019
 *      Author: Carson
 */

#ifndef ENROLLMENT_NODE_H_
#define ENROLLMENT_NODE_H_

bool Enrollment(uint8_t node_id, uint8_t *secret_key,
                uint8_t *ServerPublicKey, uint8_t *ec);

#endif /* ENROLLMENT_NODE_H_ */
