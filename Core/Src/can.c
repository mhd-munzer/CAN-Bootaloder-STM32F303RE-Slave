/*
 * can.c
 *
 *  Created on: Mar 30, 2020
 *      Author: engmh
 */

#include "can.h"

//Being added
#include "main.h"
#include <stdlib.h>
#include <string.h>
#include "flash_if.h"
#include "ymodem.h"

#define DEVICE_ID                (uint8_t)0x16  //!!!!sender ID must be unique for each sender, this ID you need add to receiver array
#define RECEIVER_ID              (uint8_t)0xFE       //ID of receiver, it not need to be changed


#define NEW_FIRMWARE_SIZE_MSG       (uint8_t)0x01
#define NEW_FIRMWARE_START_MSG      (uint8_t)0x02
#define GET_FIRMWARE_VER_MSG        (uint8_t)0x03
#define GET_UNIQUE_ID_MSG           (uint8_t)0x04
#define GET_MICRO_MODE_MSG          (uint8_t)0x05
#define EXIT_BOOTLOADER_MSG         (uint8_t)0x06
#define ASSIGN_ADDRESS	            (uint8_t)0x07


#define ALL_DATA_ACCEPTED_MSG    	(uint8_t)0x0E
#define DATA_REPLY_MSG          	(uint8_t)0x00

#define BOOTLOADER_MODE	            (0xFF)

#define DEVICE_MODE		            (uint8_t)0x10
#define FIRMWARE_VER                (uint8_t)0x11
#define UNIQUE_ID				    (uint8_t)0x12

#define STM32_UUID ((uint32_t *)0x1FFF7A10)

static uint32_t NewFirmwareSize =0;

static void can_parse_header(uint32_t can_header,uint8_t* device_ID,uint8_t* sensor_ID, uint8_t* sensor_type,uint8_t* message_type);
static uint32_t can_generate_header(uint8_t device_ID,uint8_t sensor_ID, uint8_t reply_info_type,uint8_t message_type);
static void	can_GetUniqueID(uint32_t* idPart);
static void can_GET_FIRWARE_START(uint32_t NewFirmwareSize);
static COM_StatusTypeDef can_download(void);
//static uint16_t can_read_packet(uint32_t* source);


//

extern CAN_HandleTypeDef hcan;
extern CAN_TxHeaderTypeDef pTxHeader;
extern CAN_RxHeaderTypeDef pRxHeader;
extern uint32_t TxMailBox;
extern uint8_t Txbuffer[8];
extern uint8_t Rxbuffer[8];


static void can_GetUniqueID(uint32_t* idPart){
     idPart[0] = STM32_UUID[0];
     idPart[1] = STM32_UUID[1];
     idPart[2] = STM32_UUID[2];
     //idpart[3] = STM32_UUID[3];
}

static void can_parse_header(uint32_t can_header,uint8_t* device_ID,uint8_t* sensor_ID, uint8_t* sensor_type,uint8_t* message_type){
    *device_ID=(uint8_t)((can_header&0x00FF0000)>>16);
    *sensor_ID=(uint8_t)((can_header&0xFF000000)>>24);
    *sensor_type=(uint8_t)(can_header&0x000000FF);
    *message_type=(uint8_t)((can_header&0x0000FF00)>>8);
}

uint32_t can_generate_header(uint8_t device_ID,uint8_t sensor_ID, uint8_t reply_info_type,uint8_t message_type){
	uint32_t tmp,res;
	tmp=0;
	res=0;
	tmp=device_ID;
	tmp<<=8;
	res|=tmp;
	tmp=0;

	tmp=sensor_ID;
	tmp<<=16;
	res|=tmp;
	tmp=0;

	tmp=reply_info_type;
	res|=tmp;
	tmp=0;

	tmp=message_type;
	tmp<<=4;
	res|=tmp;
	tmp=0;

	return res;
}

void CAN_Get_Command(uint8_t* Rxbuffer){

//uint8_t length = Rxbuffer[0];
//pRxHeader.ExtId
//	memset(Txbuffer,0x12,8);
//	switch (Rxbuffer[1]){
//		case 0x51:
//			HAL_CAN_AddTxMessage(&hcan, &pTxHeader, Txbuffer, &TxMailBox);
//			break;
//		default:
//			break;

uint32_t temp;
uint32_t msg;
uint32_t msg_long[3];
uint8_t msgType,devID,sensorID,sensorType;


temp=pRxHeader.ExtId;
can_parse_header(temp,&devID,&sensorID,&sensorType,&msgType);

//if message from receiver
if(devID==RECEIVER_ID){
//if message to me
	if(sensorID == DEVICE_ID){
		switch(msgType){
			case GET_MICRO_MODE_MSG:
				msg = BOOTLOADER_MODE;
				pTxHeader.ExtId = can_generate_header(DEVICE_ID,0,DEVICE_MODE,DATA_REPLY_MSG);
				memcpy(Txbuffer,&msg,sizeof(msg));
				pTxHeader.DLC= sizeof(msg);
				HAL_CAN_AddTxMessage(&hcan, &pTxHeader, Txbuffer, &TxMailBox);
			case GET_FIRMWARE_VER_MSG:
				msg = FlashReadFirmwareVersion();
				pTxHeader.ExtId=can_generate_header(DEVICE_ID,0,FIRMWARE_VER,DATA_REPLY_MSG);
				memcpy(Txbuffer,&msg,sizeof(msg));
				pTxHeader.DLC= sizeof(msg);
				HAL_CAN_AddTxMessage(&hcan, &pTxHeader, Txbuffer, &TxMailBox);
				break;
			case GET_UNIQUE_ID_MSG:
				can_GetUniqueID(msg_long);
				pTxHeader.ExtId=can_generate_header(DEVICE_ID,0,UNIQUE_ID,DATA_REPLY_MSG);
				memcpy(Txbuffer,&msg,sizeof(msg_long));
				pTxHeader.DLC= sizeof(msg_long);
				HAL_CAN_AddTxMessage(&hcan, &pTxHeader, Txbuffer, &TxMailBox);
				break;
			case NEW_FIRMWARE_SIZE_MSG:
				NewFirmwareSize = *((uint32_t *)(Rxbuffer+1));
				can_GET_FIRWARE_START(NewFirmwareSize); // next message has to be the NEW_FIRMWARE_START_MSG if not exit

				break;
//			case ASSIGN_ADDRESS:
//				FlashWriteCANAdddress(can1RxMessage.Data);
//				break;
//if data request send value from array
			case EXIT_BOOTLOADER_MSG:
				boot_loader_jump_to_user_app();
				break;
// do nothing, just return
			case ALL_DATA_ACCEPTED_MSG:
				break;
			default:
				break;
}
}

}
}

static void can_GET_FIRWARE_START(uint32_t NewFirmwareSize){


	// next message has to be the NEW_FIRMWARE_START_MSG if not exit
	uint32_t temp;
	uint8_t msgType,devID,sensorID,sensorType;
	while(!HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0));
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &pRxHeader, Rxbuffer);
	temp=pRxHeader.ExtId;
	can_parse_header(temp,&devID,&sensorID,&sensorType,&msgType);

		//if message from receiver
		if(devID==RECEIVER_ID)
		{
			//if message to me
			if(sensorID == DEVICE_ID)
			{
				if (msgType == NEW_FIRMWARE_START_MSG){
					if(can_download()!=COM_OK){
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);

						//main_download_error();
					}
					else{
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);
						HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
						HAL_Delay(1000);

					}
				}
			}
		}
	}

static COM_StatusTypeDef can_download(void){

	uint32_t size = 0;
	COM_StatusTypeDef status = COM_OK;

	status = Ymodem_Receive(&size);
	boot_loader_manage();
	return status;
}






