/*
 * QAMGenFS2019.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos, Michael Meier, Cedric Häuptli
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"
#include "ButtonHandler.h"


#define Settings_QAM_Ordnung			1<<0
#define Settings_Source_Bit1			1<<1
#define Settings_Source_Bit2			1<<2
#define Settings_Frequenz				1<<3

#define Status_Error					1<<1
#define Status_Daten_ready				1<<2
#define Status_Daten_sending			1<<3


extern void vApplicationIdleHook( void );
void vMenu(void *pvParameters);

TaskHandle_t Menu;

EventGroupHandle_t xSettings;
EventGroupHandle_t xStatus;


void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	xTaskCreate( vMenu, (const char *) "Menu", configMINIMAL_STACK_SIZE, NULL, 1, &Menu);
	
	xSettings = xEventGroupCreate();
	xStatus = xEventGroupCreate();
	
	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"Template");
	vDisplayWriteStringAtPos(3,0,"ResetReason: %d", reason);
	vTaskStartScheduler();
	return 0;
}
/*---------------------------------------------------------------------------------------------------------------*/
/* vMenu ist zuständig für die Ausgabe am Display und das Handhaben der Einstellungen durch die Buttonseingabe  */
/*---------------------------------------------------------------------------------------------------------------*/
void vMenu(void *pvParameters) {
	(void) pvParameters;
	uint8_t ucSettings = 0;
	uint8_t ucSource = 0;
	uint8_t ueDisplayUpdateTimer= 0;
	uint8_t ucModus= 0;
	
	for(;;) 
	{
		updateButtons();
		
/*--------Button 1 ist für das Auswählen der Einstellung --------*/
		if (getButtonPress(BUTTON1)== PRESSED)
		{
			if (ucModus == 1)
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
		
/*--------Button 2 ist für das wechseln der gewählten Einstellung --------*/			
		if (getButtonPress(BUTTON2)== PRESSED)
		{
			if (ucModus == 1)
			{
				if (ucSettings == 0)
				{
					if ((xEventGroupGetBits(xSettings) & Settings_QAM_Ordnung) == 0)
					{
						xEventGroupSetBits(xSettings,Settings_QAM_Ordnung);
					}
					else
					{
						xEventGroupClearBits(xSettings,Settings_QAM_Ordnung);
					}
				}
				
				if (ucSettings == 1)
				{
					if (ucSource < 2 )
					{
						ucSource++;
					}
					else
					{
						ucSource = 0;
					}
				}
				
				if (ucSettings == 2)
				{
					if ((xEventGroupGetBits(xSettings) & Settings_Frequenz) == 0)
					{
						xEventGroupSetBits(xSettings,Settings_Frequenz);
					}
					else
					{
						xEventGroupClearBits(xSettings,Settings_Frequenz);
					}
				}
			}
		}
	
/*--------Button 3 ist für das Auwählen der Modi --------*/
		if (getButtonPress(BUTTON3)== PRESSED)
		{
			if (ucModus < 1)
			{
				ucModus++;
			}
			else
			{
				ucModus = 0;
			}
			
		}
		/*if (getButtonPress(BUTTON4)== PRESSED)
		{
			
		}
		*/
		
		if (ucSource == 0)
		{
			xEventGroupSetBits(xSettings,Settings_Source_Bit1);
			xEventGroupClearBits(xSettings,Settings_Source_Bit2);
		}
		else if (ucSource == 1)
		{
			xEventGroupClearBits(xSettings,Settings_Source_Bit1);
			xEventGroupSetBits(xSettings,Settings_Source_Bit2);
		}
		else if (ucSource == 2)
		{
			xEventGroupSetBits(xSettings,Settings_Source_Bit1 | Settings_Source_Bit2);
		}
		
/*------Displayausgabe, diese wird alle 500 ms aktualisiert--------*/		
		if (ueDisplayUpdateTimer > 50) 
		{
			ueDisplayUpdateTimer = 0;
			
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0,"QAMGen2019");
			
			/*------Modus 0 wird der Status Ausgegeben--------*/	
			if (ucModus == 0)
			{
				vDisplayWriteStringAtPos(1,0,"Status");
				if ((xEventGroupGetBits(xStatus) & Status_Error) == 0)
				{
					vDisplayWriteStringAtPos(1,14,"Error");
				}
				else
				{
					vDisplayWriteStringAtPos(1,14,"Ready");
				}
			}
			
			/*------Modus 1 werden die Einstellungen ausgegeben--------*/	
			if (ucModus == 1)
			{
				vDisplayWriteStringAtPos(1,1,"QAMOrdnung");
				if ((xEventGroupGetBits(xSettings) & Settings_QAM_Ordnung) == 0)
				{
					vDisplayWriteStringAtPos(1,14,"4QAM");
				}
				else
				{
					vDisplayWriteStringAtPos(1,14,"16QAM");
				}
				
				vDisplayWriteStringAtPos(2,1,"Source");
				if ((xEventGroupGetBits(xSettings) & (Settings_Source_Bit1 | Settings_Source_Bit2)) == Settings_Source_Bit1)
				{
					vDisplayWriteStringAtPos(2,14,"I2C");
				}
				else if ((xEventGroupGetBits(xSettings) & (Settings_Source_Bit1 | Settings_Source_Bit2)) == Settings_Source_Bit2)
				{
					vDisplayWriteStringAtPos(2,14,"TEST");
				}
				else if ((xEventGroupGetBits(xSettings) & (Settings_Source_Bit1 | Settings_Source_Bit2)) == Settings_Source_Bit1 | Settings_Source_Bit2)
				{
					vDisplayWriteStringAtPos(2,14,"UART");
				}
				
				vDisplayWriteStringAtPos(3,1,"Frequenz");
				if ((xEventGroupGetBits(xSettings) & Settings_Frequenz) == 0)
				{
					vDisplayWriteStringAtPos(3,14,"100Hz");
				}
				else 
				{
					vDisplayWriteStringAtPos(3,14,"1kHz");
				}
			vDisplayWriteStringAtPos(ucSettings+1,0,"-");	
			}
		}
		ueDisplayUpdateTimer++;
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}
