/*
 * twiMaster.c
 *
 * Created: 04.12.2017 23:32:22
 *  Author: Martin Burger
 */ 

 #include <avr/io.h>
 #include "twiMaster.h"

/**
* ucI2cControl is the Control of the I2c Master
* @param ucAck => send Acknowlegde , xCmd => command for I2c
* @return is it working
* @author m.burger,C.Häuptli
*/
 uint8_t ucI2cControl(uint8_t ucAck, i2cCmd_t xCmd) {
	 ucAck = (ucAck > 0?1:0);
	 ucAck<<=2;
	 return ucAck | (xCmd & 0x03);
 }

/**
* vInitI2C is to initialization the I2C
* @param args Unused
* @return Nothing
* @author m.burger,C.Häuptli
*/

 void vInitI2C(void) {
	 PORTC.DIRSET = 0x03;
	 PORTC.OUT = 0x03;	 
	 TWIC.MASTER.CTRLC = ucI2cControl(1, eNOACT);	 
	 TWIC.MASTER.BAUD = TWI_BAUDSETTING;
	 TWIC.MASTER.CTRLA = TWI_MASTER_ENABLE_bm;
	 TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
 }
 
/**
* vI2cRead is to read from I2C
* @param ucAdd => Slave Adress, ucReg => Register to read, ucN => How Many Byte read, pucData => pionter to the data
* @return Nothing
* @author m.burger, C.Häuptli
*/

 void vI2cRead(uint8_t ucAdd, uint8_t ucReg, uint8_t ucN, uint8_t* pucData) {
	 TWIC.MASTER.ADDR = ucAdd;
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 TWIC.MASTER.DATA = (ucReg);
	 TWIC.MASTER.ADDR = (ucAdd | 0x01);
	 TWIC.MASTER.CTRLC = ucI2cControl((ucN>1?1:0), eSTART);
	 for(int i = 0; i < ucN; i++) {
		 while(!(TWIC.MASTER.STATUS&TWI_MASTER_RIF_bm));
		 pucData[i] = TWIC.MASTER.DATA;
		 TWIC.MASTER.CTRLC = ucI2cControl((i>=ucN-1?1:0), (i>=ucN-1?eSTOP:eBYTEREC));
	 }	 
 }
 
 /**
 * ucI2cReadByte is to read only one Byte from the I2C
 * @param ucAdd => Slave Adress, ucReg => Register to read
 * @return the data from the I2C
 * @author m.burger, C.Häuptli
 */

 uint8_t ucI2cReadByte(uint8_t ucAdd, uint8_t ucReg) {
	 TWIC.MASTER.ADDR = ucAdd;
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 TWIC.MASTER.DATA = (ucReg);
	 TWIC.MASTER.ADDR = (ucAdd | 0x01);
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_RIF_bm));
	 uint8_t returndata = TWIC.MASTER.DATA;
	 TWIC.MASTER.CTRLC = ucI2cControl(0, eSTOP);
	 return returndata;
 }
 
 /**
 * vI2cWrite is to write to the I2C
 * @param ucAdd => Slave Adress, ucReg => Register to read, ucN => How Many Byte read, pucData => pionter to the data
 * @return Nothing
 * @author m.burger, C.Häuptli
 */

 void vI2cWrite(uint8_t ucAdd, uint8_t ucReg, uint8_t ucN, uint8_t* pucData) {
	 TWIC.MASTER.ADDR = ucAdd;
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 TWIC.MASTER.DATA = (ucReg);
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 for(int i = 0; i < ucN; i++) {
		 TWIC.MASTER.DATA = pucData[i];
		 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 }
	 TWIC.MASTER.CTRLC = ucI2cControl(0, eSTOP);
 }
 
/**
* vI2cWriteByte is to write only one Byte to the i2c
* @param ucAdd => Slave Adress, ucReg => Register to read, ucData => data to send
* @return Nothing
* @author m.burger, C.Häuptli
*/

 void vI2cWriteByte(uint8_t ucAdd, uint8_t ucReg, uint8_t ucData) {
	 TWIC.MASTER.ADDR = ucAdd;
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 TWIC.MASTER.DATA = (ucReg);
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 TWIC.MASTER.DATA = ucData;
	 while(!(TWIC.MASTER.STATUS&TWI_MASTER_WIF_bm));
	 TWIC.MASTER.CTRLC = ucI2cControl(0, eSTOP);
 }