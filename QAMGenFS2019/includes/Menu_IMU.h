/**
 * Menu_IMU.h
 *
 * This is the header Methode which generate the display output and organize the IMU input.
 * @Author C. Häuptli
 */ 
#ifndef Menu_IMU_H_
#define Menu_IMU_H_

#include "ButtonHandler.h"
#include "twiMaster.h"
#include "LSM9DS1Defines.h"

#include "NHD0420Driver.h"

 #include "FreeRTOS.h"
 #include "task.h"
 #include "event_groups.h"
 #include "queue.h"
 #include "semphr.h"
 #include "protocolhandler.h"


#define STATUS_ERROR				1<<0
#define STATUS_DATA_READY			1<<1
#define STATUS_DATA_SENDING			1<<2

TaskHandle_t xMenu;
TaskHandle_t xIMU;
TaskHandle_t xTestpattern;
TaskHandle_t xIO;

extern volatile uint8_t	ucCPULoad;

SemaphoreHandle_t xSettingKey;		//Lock for Settings
SemaphoreHandle_t xStatusKey;		//Lock for Status

QueueHandle_t xData;
QueueHandle_t xDatabriged;

 union QAM_SETTING
 {
	 struct
	 {
		 uint8_t bQAM_Order:1;
		 uint8_t bSource_I2C:1;
		 uint8_t bSource_Test:1;
		 uint8_t bSource_UART:1;
		 uint8_t bOutput_IO:1;
		 uint8_t bFrequency:1;
		 uint8_t bBit7:1;
	 }bits;
	 uint8_t ucSettings;
 }xQAMSettings;
 
/** 
* vMenu is responsible for the display output and handles the settings of the buttons
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vMenu(void *pvParameters);

/**
* vIMU is to manage the Accelerometer and filter the data for the game
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vIMU(void *pvParameters);

/**
* vOutput is to manage the I/O Output
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vOutput(void *pvParameters);

/**
* vTestpattern is to test the QAM connection
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vTestpattern(void *pvParameters);



#endif /* Menu_IMU_H_ */
