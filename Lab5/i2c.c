/*
 * I2C Library for TM4C123GH6PM , uses I2C1
 */

#include "CU_TM4C123.h"
#include "i2c.h"
#include <stdio.h>

/*******************************************************************/
/*!
 * I2C Initialization
 * Set Baud Rate and turn on I2C1
 */
void i2c_init(void)
{
   SYSCTL->RCGCGPIO |= 1UL<<0; // Enable clock gate for port A
   SYSCTL->RCGCI2C |= 1UL<<1 ; // Enable clock gate for I2C1   
   
   GPIOA ->AFSEL |= 1UL<<6 | 1UL<<7 ; // Enable alternate function for PA6 (I2C1SCL) and PA7(I2C1SDA)
   GPIOA -> ODR |= 1UL<<7  ; // PA7(I2C1SDA) set or open drain 
   GPIOA -> PCTL |= 3UL<<(4*6) | 3UL<<(4*7); // Set alternate function for PA6 - I2C1SCL and PA7 - I2C1SDA in PMC2 and PMC3   
   GPIOA-> DEN |= 1UL<<6 | 1UL<<7 ; // Enable PA6 and PA7
   
   
   I2C1 ->MCR = 1UL<<4;   // I2C1 Master function enable
   I2C1 ->MTPR = 1; // Assuming 16MHz clock, and SCL low pulse 6 and high pulse 4 set I2C datarate to 356Kbps

}

/*******************************************************************/
/*!
 * Start transmission and reads a register from a slave device
 * @param the slaveAddress
 * @param u8RegisterAddress is Register Address
 * @return Data stored in Register
 */
int i2c_ReadRegister(uint8_t u8SlaveAddress, uint8_t u8RegisterAddress)
{
	uint8_t result = 0xFF;
	
    // Assume I2C is in Idle state
	I2C1 ->MSA = (u8SlaveAddress<<1);  // Specify the slave address and set TX
    I2C1 ->MDR = u8RegisterAddress ; // Write register address to device
    I2C1 -> MCS = I2C_MCS_START | I2C_MCS_RUN;  // Do start , transmit address and go to Master transmit state
    
    while(I2C1 -> MCS & 1UL<<0); // Wait while controller is busy
    
    I2C1 ->MSA = (u8SlaveAddress<<1)|1; // Specify the slave address and set RX       
    I2C1 -> MCS = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;  // Do Repeat start , recieve data and go to Master recieve state
    
    while(I2C1 -> MCS & 1UL<<0); // Wait while controller is busy
    
    // check if error
    if (I2C1 -> MCS & I2C_MCS_ERROR)
        return -1;
    
    result = I2C1 -> MDR;

	return result;
}

/*******************************************************************/
/*!
 *  Write a byte of data to a register on a slave device
 * @param the slaveAddress
 * @param u8RegisterAddress is Register Address
 * @param u8Data is Data to write
 */
int i2c_WriteRegister(uint8_t u8SlaveAddress, uint8_t u8RegisterAddress, int8_t u8Data)
{
    // Assume I2C is in Idle state
	I2C1 ->MSA = (u8SlaveAddress<<1);  // Specify the slave address and set TX
    
    I2C1 ->MDR = u8RegisterAddress ; // Write register address to device
  
    I2C1 -> MCS = I2C_MCS_START | I2C_MCS_RUN;  // Do start , transmit address and go to Master transmit state
    
    while(I2C1 -> MCS & 1UL<<0); // Wait while controller is busy
    
    I2C1 ->MDR = u8Data;
        
    I2C1 -> MCS  = I2C_MCS_RUN | I2C_MCS_STOP;

    while(I2C1 -> MCS & 1UL<<0); // Wait while controller is busy

    // check if error
    if (I2C1 -> MCS & I2C_MCS_ERROR)
        return -1;
    else
        return 0;
}

