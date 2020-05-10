/*
 * can.h
 *
 *  Created on: Mar 30, 2020
 *      Author: engmh
 */

#ifndef INC_CAN_H_
#define INC_CAN_H_

#include "main.h"
#include "ymodem.h"

typedef void (*pFunction)(void);
void CAN_Get_Command(uint8_t* Rxbuffer);
static void can_parse_header(uint32_t can_header,uint8_t* device_ID,uint8_t* sensor_ID, uint8_t* sensor_type,uint8_t* message_type);
static uint32_t can_generate_header(uint8_t device_ID,uint8_t sensor_ID, uint8_t reply_info_type,uint8_t message_type);
static void	can_GetUniqueID(uint32_t* idPart);
static void can_GET_FIRWARE_START(uint32_t Data);
static COM_StatusTypeDef can_download(void);
//static uint16_t can_read_packet(uint32_t* source);


#endif /* INC_CAN_H_ */
