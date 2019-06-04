/*
 * twiMaster.h
 *
 * Created: 04.12.2017 23:32:36
 *  Author: Martin Burger, C. Häuptli
 */ 


#ifndef TWIMASTER_H_
#define TWIMASTER_H_


#define CPU_SPEED 32000000
#define BAUDRATE    400000
#define TWI_BAUD(F_SYS, F_TWI) ((F_SYS / (2 * F_TWI)) - 5)
#define TWI_BAUDSETTING TWI_BAUD(CPU_SPEED, BAUDRATE)

typedef enum i2cCmd_Tag{
	eNOACT = 0,
	eSTART = 1,
	eBYTEREC = 2,
	eSTOP = 3
}i2cCmd_t;

/**
* vInitI2C is to initialization the I2C
* @param args Unused
* @return Nothing
* @author m.burger,C.Häuptli
*/

void vInitI2C(void);

/**
* vI2cRead is to read from I2C
* @param ucAdd => Slave Adress, ucReg => Register to read, ucN => How Many Byte read, pucData => pionter to the data
* @return Nothing
* @author m.burger, C.Häuptli
*/

void vI2cRead(uint8_t ucAdd, uint8_t ucReg, uint8_t ucN, uint8_t* pucData);

/**
* ucI2cReadByte is to read only one Byte from the I2C
* @param ucAdd => Slave Adress, ucReg => Register to read
* @return the data from the I2C
* @author m.burger, C.Häuptli
*/

uint8_t ucI2cReadByte(uint8_t ucAdd, uint8_t ucReg);

/**
* vI2cWrite is to write to the I2C
* @param ucAdd => Slave Adress, ucReg => Register to read, ucN => How Many Byte read, pucData => pionter to the data
* @return Nothing
* @author m.burger, C.Häuptli
*/

void vI2cWrite(uint8_t ucAdd, uint8_t ucReg, uint8_t ucN, uint8_t* pucData);

/**
* vI2cWriteByte is to write only one Byte to the i2c
* @param ucAdd => Slave Adress, ucReg => Register to read, ucData => data to send
* @return Nothing
* @author m.burger, C.Häuptli
*/
void vI2cWriteByte(uint8_t ucAdd, uint8_t ucReg, uint8_t ucData);

#endif /* TWIMASTER_H_ */