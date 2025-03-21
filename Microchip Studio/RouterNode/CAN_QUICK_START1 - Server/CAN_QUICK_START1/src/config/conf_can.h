/**
 * \file
 *
 * \brief SAM Control Area Network Driver Configuration Header
 *
 * Copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#ifndef CONF_CAN_H_INCLUDED
#define CONF_CAN_H_INCLUDED

/*
 * Below is the message RAM setting, it will be stored in the system RAM.
 * Please adjust the message size according to your application.
 */
#define CONF_CAN0_RX_FIFO_0_NUM         16            /* Range: 1..64 */ 
#define CONF_CAN0_RX_FIFO_1_NUM         16            /* Range: 1..64 */ 
#define CONF_CAN0_RX_BUFFER_NUM         16            /* Range: 1..64 */ 
#define CONF_CAN0_TX_BUFFER_NUM         4             /* Range: 1..16 */
#define CONF_CAN0_TX_FIFO_QUEUE_NUM     4             /* Range: 1..16 */
#define CONF_CAN0_TX_EVENT_FIFO         8             /* Range: 1..32 */

#define CONF_CAN0_RX_STANDARD_ID_FILTER_NUM     32    /* Range: 1..128 */
#define CONF_CAN0_RX_EXTENDED_ID_FILTER_NUM     16    /* Range: 1..64 */

#define CONF_CAN1_RX_FIFO_0_NUM         16            /* Range: 1..64 */ 
#define CONF_CAN1_RX_FIFO_1_NUM         16            /* Range: 1..64 */ 
#define CONF_CAN1_RX_BUFFER_NUM         16            /* Range: 1..64 */ 
#define CONF_CAN1_TX_BUFFER_NUM         4             /* Range: 1..16 */
#define CONF_CAN1_TX_FIFO_QUEUE_NUM     4             /* Range: 1..16 */
#define CONF_CAN1_TX_EVENT_FIFO         8             /* Range: 1..32 */

#define CONF_CAN1_RX_STANDARD_ID_FILTER_NUM     32    /* Range: 1..128 */
#define CONF_CAN1_RX_EXTENDED_ID_FILTER_NUM     16    /* Range: 1..64 */

/* The value should be 8/12/16/20/24/32/48/64. */
#define CONF_CAN_ELEMENT_DATA_SIZE         64

/*
 * The setting of the nominal bit rate is based on the GCLK_CAN is 48M which you can
 * change in the conf_clock.h. Below is the default configuration. The
 * time quanta is 48MHz / (5+1) =  8MHz. And each bit is (1 + NTSEG1 + 1 + NTSEG2 + 1) = 16 time
 * quanta which means the bit rate is 8MHz/16=500KHz.
 */
/* Nominal bit Baud Rate Prescaler */
#define CONF_CAN_NBTP_NBRP_VALUE    5
/* Nominal bit (Re)Synchronization Jump Width */
#define CONF_CAN_NBTP_NSJW_VALUE    3
/* Nominal bit Time segment before sample point */
#define CONF_CAN_NBTP_NTSEG1_VALUE  10
/* Nominal bit Time segment after sample point */
#define CONF_CAN_NBTP_NTSEG2_VALUE  3

/*
 * The setting of the data bit rate is based on the GCLK_CAN is 48M which you can
 * change in the conf_clock.h. Below is the default configuration. The
 * time quanta is 48MHz / (5+1) =  8MHz. And each bit is (1 + DTSEG1 + 1 + DTSEG2 + 1) = 16 time
 * quanta which means the bit rate is 8MHz/16=500KHz.
 */
/* Data bit Baud Rate Prescaler */
#define CONF_CAN_DBTP_DBRP_VALUE    0
/* Data bit (Re)Synchronization Jump Width */
#define CONF_CAN_DBTP_DSJW_VALUE    3
/* Data bit Time segment before sample point */
#define CONF_CAN_DBTP_DTSEG1_VALUE  10
/* Data bit Time segment after sample point */
#define CONF_CAN_DBTP_DTSEG2_VALUE  3

#endif
