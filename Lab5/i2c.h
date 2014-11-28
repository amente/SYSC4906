/*
 * I2C.h
 *
 *  Created on: Aug 31, 2014
 *      Author: amente
 */

#ifndef I2C_H_
#define I2C_H_

#include "CU_TM4C123.h"
#include <stdint.h>


void i2c_init(void);
int i2c_WriteRegister(uint8_t u8SlaveAddress,uint8_t u8RegisterAddress, int8_t u8Data);
int i2c_ReadRegister(uint8_t u8SlaveAddress, uint8_t u8RegisterAddress);
// void i2c_ReadMultiRegisters(uint8_t u8SlaveAddress, uint8_t u8RegisterAddress, uint8_t n, uint8_t *r);



#endif /* I2C_H_ */
