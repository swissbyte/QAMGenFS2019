/*
 * protocolhandler.h
 *
 * Created: 18.05.2019 17:23:41
 *  Author: PhilippAdmin
 */ 


#ifndef PROTOKOLLHANDLER_H_
#define PROTOKOLLHANDLER_H_
#define	MAX_FRAME_SIZE				32
#include "semphr.h"


struct ALDP_t_class
{
	uint8_t aldp_hdr_byte_1;
	uint8_t aldp_size;
	uint8_t aldp_payload[10];
};

#define GLOBAL_FRAME_PREAMBLE	0x55A1

/* xQuelle */
#define PAKET_TYPE_ALDP					0x40
#define ALDP_SRC_UART					0x00
#define ALDP_SRC_I2C					0x01
#define ALDP_SRC_TEST					0x02

struct SLDP_t_class
{
	uint8_t sldp_size;
	uint8_t *sldp_payload;
	uint8_t sldp_crc8;
};

extern SemaphoreHandle_t xGlobalFrameBuffer_A_Key;			//A-Resource for ucGlobalFrameBuffer_A
extern SemaphoreHandle_t xGlobalFrameBuffer_B_Key;			//A-Resource for ucGlobalFrameBuffer_B
extern volatile uint8_t ucGlobalFrameBuffer_A[ MAX_FRAME_SIZE ];
extern volatile uint8_t ucGlobalFrameBuffer_B[ MAX_FRAME_SIZE ];

extern xQueueHandle xALDPQueue;								// Data to pack and send

void vProtokollHandlerTask( void *pvParameters );

#endif /* PROTOKOLLHANDLER_H_ */