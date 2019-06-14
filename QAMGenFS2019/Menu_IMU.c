/**
 * Menu_IMU.c
 *
 * This is the main Methode which generate the display output and organize the IMU input.
 * @Author C. Häuptli
 */ 

 #include <avr/io.h>
 #include "Menu_IMU.h"
 

uint32_t ulStatus = 0;				//P-Resource Status
 

/**
* vMenu is responsible for the display output and handles the settings of the buttons
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vMenu(void *pvParameters) {
	(void) pvParameters;
	uint8_t ucSettings = 0;
	uint8_t ucSource = 0;
	uint8_t ucDisplayUpdateTimer= 0;
	uint8_t ucMode= 0;
	uint8_t ucData_to_send = 0;
	
	xQAMSettings.ucSettings = 0x02; //Start Settings for QAM

	
	for(;;) 
	{
		updateButtons();
	
	
		/*--------Button 1 is to select the settings--------*/	
		if (getButtonPress(eBUTTON1)== ePRESSED)
		{
			if (ucMode == 1)
			{
				if (ucSettings < 2)
				{
					ucSettings++;
				}
				else
				{
					ucSettings = 0;
				}
			}
		}
		
		/*--------Button 2 is to change the selected setting--------*/			
		if (getButtonPress(eBUTTON2)== ePRESSED)
		{
			if (ucMode == 1)
			{
				if (ucSettings == 0)
				{
					if(xSemaphoreTake(xSettingKey, portMAX_DELAY))
					{
						if (xQAMSettings.bits.bQAM_Order == 1)
						{
							xQAMSettings.bits.bQAM_Order = 0;
						}
						else
						{
							xQAMSettings.bits.bQAM_Order = 1;
						}
						xSemaphoreGive(xSettingKey);
					}
				}
				
				if (ucSettings == 1)
				{
					if (ucSource < 3 )
					{
						ucSource++;
					}
					else
					{
						ucSource = 0;
					}
					if(xSemaphoreTake(xSettingKey, portMAX_DELAY))
					{
						xQAMSettings.bits.bSource_I2C = 0;
						xQAMSettings.bits.bSource_Test = 0;
						xQAMSettings.bits.bSource_UART = 0;
						xQAMSettings.bits.bOutput_IO = 0;
						switch (ucSource)
						{
							case 0:
								xQAMSettings.bits.bSource_I2C = 1;
								vTaskResume(xIMU);
								vTaskSuspend(xTestpattern);
								vTaskSuspend(xIO);
								//vTaskSuspend(xUART)
								break;
							case 1:
								xQAMSettings.bits.bSource_Test = 1;
								vTaskResume(xTestpattern);
								vTaskSuspend(xIMU);
								vTaskSuspend(xIO);
								//vTaskSuspend(xUART);
								break;
							case 2:
								xQAMSettings.bits.bSource_UART = 1;
								//vTaskResume(xUART);
								vTaskSuspend(xIMU);
								vTaskSuspend(xTestpattern);
								vTaskSuspend(xIO);
								break;
							case 3:
								xQAMSettings.bits.bOutput_IO = 1;
								//vTaskResume(xUART);
								vTaskResume(xIMU);
								vTaskResume(xIO);
								vTaskSuspend(xTestpattern);
								break;
							default:break;
						}
						xSemaphoreGive(xSettingKey);
					}
				}
				
				if (ucSettings == 2)
				{
					if(xSemaphoreTake(xSettingKey, portMAX_DELAY))
					{
						if(xQAMSettings.bits.bFrequency)
						{
							xQAMSettings.bits.bFrequency = 0;
						}
						else
						{
							xQAMSettings.bits.bFrequency = 1;
						}
						xSemaphoreGive(xSettingKey);
					}
				}
			}
		}
	
		/*--------Button 3 is to select the Mode --------*/
		if (getButtonPress(eBUTTON3)== ePRESSED)
		{
			if (ucMode < 2)
			{
				ucMode++;
			}
			else
			{
				ucMode = 0;
			}
			
		}

		/*--------Button 4 is not definded --------*/
		/*if (getButtonPress(BUTTON4)== PRESSED)
		{
			
		}
		*/
		
		/*------Displayoutput will update all 500ms--------*/		
		if (ucDisplayUpdateTimer > 50) 
		{
			ucDisplayUpdateTimer = 0;
			
			vDisplayClear();
			
			
			/*------Mode 0 will show the state--------*/	
			if (ucMode == 0)
			{
				vDisplayWriteStringAtPos(0,0,"QAMGen2019");
				vDisplayWriteStringAtPos(2,0,"Status");
				vDisplayWriteStringAtPos(3,0,"CPU:");
				vDisplayWriteStringAtPos(3,5,"%d%%", ucCPULoad);
				
				
				if(xSemaphoreTake(xStatusKey, 5 / portTICK_RATE_MS))
				{
					if((ulStatus && STATUS_ERROR)== STATUS_ERROR)
					{
						vDisplayWriteStringAtPos(2,14,"Ready");
					}
					else
					{
						vDisplayWriteStringAtPos(2,14,"Error");
					}
					xSemaphoreGive(xStatusKey);
				}
			}
			
			/*------Mode 1 will show the setting--------*/	
			if (ucMode == 1)
			{
				vDisplayWriteStringAtPos(0,0,"QAM Setting");
				vDisplayWriteStringAtPos(1,1,"QAMOrdnung");
				vDisplayWriteStringAtPos(2,1,"Source");
				vDisplayWriteStringAtPos(3,1,"Frequenz");
				if(xSemaphoreTake(xSettingKey, 5 / portTICK_RATE_MS))
				{
					if (xQAMSettings.bits.bQAM_Order)
					{
						vDisplayWriteStringAtPos(1,14,"16QAM");
					}
					else
					{
						vDisplayWriteStringAtPos(1,14,"4QAM");
					}
					
					if (xQAMSettings.bits.bSource_I2C)
					{
						vDisplayWriteStringAtPos(2,14,"I2C");
					}
					else if (xQAMSettings.bits.bSource_Test)
					{
						vDisplayWriteStringAtPos(2,14,"TEST");
					}
					else if (xQAMSettings.bits.bSource_UART)
					{
						vDisplayWriteStringAtPos(2,14,"UART");
					}
					else if (xQAMSettings.bits.bOutput_IO)
					{
						vDisplayWriteStringAtPos(2,14,"Briged");
					}
					
					if (xQAMSettings.bits.bFrequency)
					{
						vDisplayWriteStringAtPos(3,14,"100Hz");
					}
					else
					{
						vDisplayWriteStringAtPos(3,14,"1kHz");
					}
					xSemaphoreGive(xSettingKey);
				}
				vDisplayWriteStringAtPos(ucSettings+1,0,"-");	
			}
			
			/*------Mode 2 will show the data-------*/	
			if (ucMode == 2)
			{
				vDisplayWriteStringAtPos(0,0,"QAM Data");
				if(xQueuePeek(xALDPQueue,&ucData_to_send,5/portTICK_RATE_MS))
				{
					vDisplayWriteStringAtPos(1,0,"Data: %d", ucData_to_send);
				}
				else
				{
					vDisplayWriteStringAtPos(1,0,"Data: No Data ");
				}
				
				
			}
				
		}
		ucDisplayUpdateTimer++;
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

/**
* vIMU is to manage the Accelerometer and filter the data for the game
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vIMU(void *pvParameters) {
	(void) pvParameters;
	
	vInitI2C();
	
	vI2cWriteByte(GYROACCADDRESS, CTRL_REG5_XL, 0xF8); //Activate the Accelerometer
	vI2cWriteByte(GYROACCADDRESS, CTRL_REG6_XL, 0xC0); //Set the sampling rate to 952Hz
	
	int16_t usAccel_x = 0;
	int16_t usAccel_y = 0;
	
	uint8_t ucData_Accel_x_positiv = 0;
	uint8_t ucData_Accel_x_negativ = 0;
	uint8_t ucData_Accel_y_positiv = 0;
	uint8_t ucData_Accel_y_negativ = 0;
	uint8_t ucData_to_send = 0;
	
	
	for(;;) {
		
		uint8_t usTemp[4];
		vI2cRead(GYROACCADDRESS, OUT_X_L_XL, 4, usTemp);
		usAccel_x = (usTemp[1] << 8)| usTemp[0];
		usAccel_y = (usTemp[3] << 8)| usTemp[2];

		
		if ((32000> usAccel_x) && (usAccel_x > 3000))
		{
			ucData_Accel_x_positiv = 3;
		}
		else if ((3000> usAccel_x) && (usAccel_x > 2000))
		{
			ucData_Accel_x_positiv = 2;
		}
		else if ((2000> usAccel_x) && (usAccel_x > 1000))
		{
			ucData_Accel_x_positiv = 1;
		}
		else
		{
			ucData_Accel_x_positiv = 0;
		}
		
		if ((-32000 < usAccel_x) && (usAccel_x < -3000))
		{
			ucData_Accel_x_negativ = 3;
		}
		else if ((-3000 < usAccel_x) && (usAccel_x < -2000))
		{
			ucData_Accel_x_negativ = 2;
		}
		else if ((-2000 < usAccel_x) && (usAccel_x < -1000)) 
		{
			ucData_Accel_x_negativ = 1;
		}
		else 
		{
			ucData_Accel_x_negativ = 0;
		}
		
		
		if ((32000> usAccel_y) && (usAccel_y > 3000))
		{
			ucData_Accel_y_positiv = 3;
		}
		else if ((3000> usAccel_y) && (usAccel_y > 2000))
		{
			ucData_Accel_y_positiv = 2;
		}
		else if ((2000> usAccel_y) && (usAccel_y > 1000))
		{
			ucData_Accel_y_positiv = 1;
		}
		else
		{
			ucData_Accel_y_positiv = 0;
		}
		
		if ((-32000 < usAccel_y) && (usAccel_y < -3000))
		{
			ucData_Accel_y_negativ = 3;
		}
		else if ((-3000 < usAccel_y) && (usAccel_y < -2000))
		{
			ucData_Accel_y_negativ = 2;
		}
		else if ((-2000 < usAccel_y) && (usAccel_y < -1000))
		{
			ucData_Accel_y_negativ = 1;
		}
		else
		{
			ucData_Accel_y_negativ = 0;
		}
		ucData_to_send = (ucData_Accel_y_negativ<<6)|(ucData_Accel_y_positiv<<4)|(ucData_Accel_x_negativ<<2) | ucData_Accel_x_positiv;
		if (xQAMSettings.bits.bOutput_IO)
		{
			if (uxQueueMessagesWaiting(xDatabriged)< 2)
			{
				xQueueSendToBack(xDatabriged,&ucData_to_send,10/portTICK_RATE_MS);
			}
		}
		else
		{
			if (uxQueueMessagesWaiting(xALDPQueue)< 2)
			{
				struct ALDP_t_class xALDP_Paket;
				xALDP_Paket.aldp_size = 0x01;
				if(xQAMSettings.bits.bSource_I2C)
				{
					xALDP_Paket.aldp_hdr_byte_1 =  PAKET_TYPE_ALDP|ALDP_SRC_I2C; 
				}
				if(xQAMSettings.bits.bSource_UART)
				{
					xALDP_Paket.aldp_hdr_byte_1 = PAKET_TYPE_ALDP|ALDP_SRC_UART; 
				}
				if(xQAMSettings.bits.bSource_Test)
				{
					xALDP_Paket.aldp_hdr_byte_1 = PAKET_TYPE_ALDP|ALDP_SRC_TEST; 
				} 
				xALDP_Paket.aldp_payload[0] = ucData_to_send;
				xQueueSendToBack(xALDPQueue,(void *)&xALDP_Paket,portMAX_DELAY);
			}
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}


/**
* vOutput is to manage the I/O Output
* @param args Unused
* @return Nothing
* @author C.Haeuptli
*/
void vOutput(void *pvParameters) {
	(void) pvParameters;
	
	PORTE.DIRSET = 0xFF;
	PORTE.OUTSET = 0xFF;

	uint8_t ucDatabriged = 0;
	
	for(;;) {
		
		if(xQueueReceive(xDatabriged,&ucDatabriged,(TickType_t)100))
		{
			if (ucDatabriged & 0x01)
			{
				PORTE.OUTCLR = PIN0_bm;
			}
			else
			{
				PORTE.OUTSET = PIN0_bm;
			}
			if (ucDatabriged & 0x02)
			{
				PORTE.OUTCLR = PIN1_bm;
			}
			else
			{
				PORTE.OUTSET = PIN1_bm;
			}
			if (ucDatabriged & 0x04)
			{
				PORTE.OUTCLR = PIN2_bm;
			}
			else
			{
				PORTE.OUTSET = PIN2_bm;
			}
			if (ucDatabriged & 0x08)
			{
				PORTE.OUTCLR = PIN3_bm;
			}
			else
			{
				PORTE.OUTSET = PIN3_bm;
			}
			if (ucDatabriged & 0x10)
			{
				PORTE.OUTCLR = PIN4_bm;
			}
			else
			{
				PORTE.OUTSET = PIN4_bm;
			}
			if (ucDatabriged & 0x20)
			{
				PORTE.OUTCLR = PIN5_bm;
			}
			else
			{
				PORTE.OUTSET = PIN5_bm;
			}
			if (ucDatabriged & 0x40)
			{
				PORTE.OUTCLR = PIN6_bm;
			}
			else
			{
				PORTE.OUTSET = PIN6_bm;
			}
			if (ucDatabriged & 0x80)
			{
				PORTE.OUTCLR = PIN7_bm;
			}
			else
			{
				PORTE.OUTSET = PIN7_bm;
			}
			
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

/**
* vTestpattern is to test the QAM connection
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vTestpattern(void *pvParameters){
	(void) pvParameters;
	
	uint8_t ucTestpattern = 255;
	vTaskSuspend(xTestpattern);
	for(;;) {
		
		if (uxQueueMessagesWaiting(xALDPQueue)< 2)
		{
			xQueueSendToBack(xALDPQueue,(void *)&ucTestpattern,portMAX_DELAY);
		}
		vTaskDelay(2 / portTICK_RATE_MS);
	}
}

/**
* vUART is to send the data from the UART
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/
/*
void vUART(void *pvParameters) {
	(void) pvParameters;

	uint8_t ucUART = 256;
	
	for(;;) {
		
		xEventGroupWaitBits(xSettings,SETTING_SOURCE_BIT1,pdFALSE,pdFALSE,portMAX_DELAY);
		if (uxQueueMessagesWaiting(xData)< 2)
		{
			xQueueSendToBack(xData,&ucUART,portMAX_DELAY);
		}
	}
*/