/*
 * protocolhandler.h
 *
 * Created: 18.05.2019 17:23:41
 *  Author: PhilippAdmin
 */ 


#ifndef PROTOKOLLHANDLER_H_
#define PROTOKOLLHANDLER_H_
#define	PROTOCOLBUFFERSIZE				32
#include "semphr.h"


struct ALDP_t_class
{
	uint8_t aldp_hdr_byte_1;
	uint8_t aldp_hdr_byte_2;
	uint8_t aldp_payload[];
};

struct SLDP_t_class
{
	uint8_t sldp_size;
	uint8_t *sldp_payload;
	uint8_t sldp_crc8;
};

extern SemaphoreHandle_t xGlobalProtocolBuffer_A_Key;			//A-Resource for ucGlobalProtocolBuffer_A
extern SemaphoreHandle_t xGlobalProtocolBuffer_B_Key;			//A-Resource for ucGlobalProtocolBuffer_B
extern uint8_t ucglobalProtocolBuffer_A[ PROTOCOLBUFFERSIZE ];
extern uint8_t ucglobalProtocolBuffer_B[ PROTOCOLBUFFERSIZE ];

void vProtokollHandlerTask( void *pvParameters );

uint8_t xCRC_calc( uint8_t uiCRC, uint8_t uiCRC_data );



#endif /* PROTOKOLLHANDLER_H_ */