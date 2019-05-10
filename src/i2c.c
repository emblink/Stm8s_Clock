#include "stm8s.h"
#include "i2c.h"

#define STM8S_I2C_ADDRESS 0xFE

void i2cInit(void)
{   
    I2C->CR1 |= I2C_CR1_NOSTRETCH; // Clock stretching disable
    I2C->CR1 &= ~I2C_CR1_ENGC; // General Call Enable
    I2C->CR1 |= I2C_CR2_ACK; // ACK of the next byte which will be received in the shift register.
    I2C->FREQR |= 0x10; // 16Mhz max input frequency
    I2C->OARL |= STM8S_I2C_ADDRESS;
    I2C->OARL &= ~STM8S_I2C_ADDRESS;
    I2C->OARH &= ~I2C_OARH_ADDMODE; // 7-bit slave address (10-bit address not acknowledged)
    I2C->OARH |= I2C_OARH_ADDCONF; // This bit must set by software (must always be written as �1�).
    I2C->CCRL |= 0x50;
    I2C->CCRH &= ~I2C_CCRH_FS; // 0: Standard mode I2C
    I2C->CCRH &= ~I2C_CCRH_DUTY; // 0: Fast mode tlow/thigh = 2
    I2C->TRISER |= 0x2F; // 17 for I2C_FREQR register = 16, (1000 ns / 62.5 ns = 16 + 1)
    I2C->CR1 |= I2C_CR1_PE; // Peripheral enable
}

bool i2c_send(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t txBuff[], uint16_t txSize)
{
	if(!txBuff || !txSize)
		return FALSE;
	
	while(I2C->SR3 & I2C_SR3_BUSY); //wait for previous transaction finish
	
	I2C->CR2 |= I2C_CR2_START;  // generate start
	while (!(I2C->SR1 & I2C_SR1_SB));
	I2C->SR1; // clear SB bit
	I2C->DR = deviceAddress << 1;
	while (!(I2C->SR1 & I2C_SR1_ADDR));
	I2C->SR1; // clear ADDR bit
	I2C->SR3;
	while (!(I2C->SR1 & I2C_SR1_TXE));
	I2C->DR = deviceRegister;
	uint16_t i = 0;
	while (i < txSize) {
		while(!(I2C->SR1 & I2C_SR1_TXE));
		I2C->DR = txBuff[i++];
	}
	while(!(I2C->SR1 & I2C_SR1_TXE) && !(I2C->SR1 & I2C_SR1_BTF));
	I2C->CR2 |= I2C_CR2_STOP;
	return TRUE;
}

bool i2c_read(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t rxBuff[], uint16_t rxSize) // rads only 1 byte
{
	if(!rxBuff || !rxSize)
		return FALSE;
	
	while(I2C->SR3 & I2C_SR3_BUSY); //wait for previous transaction finish
	
	I2C->CR2 |= I2C_CR2_START;  // generate start
	while (!(I2C->SR1 & I2C_SR1_SB));
	I2C->SR1; // clear SB bit
	I2C->DR = deviceAddress << 1;
	while (!(I2C->SR1 & I2C_SR1_ADDR));
	I2C->SR1; // clear ADDR bit
	I2C->SR3;
	I2C->DR = deviceRegister;
	while (!(I2C->SR1 & I2C_SR1_TXE));
	I2C->CR2 |= I2C_CR2_START;
	while (!(I2C->SR1 & I2C_SR1_SB));
	I2C->DR = (deviceAddress << 1) | 0x01;
	while (!(I2C->SR1 & I2C_SR1_ADDR));
	I2C->CR2 &= ~I2C_CR2_ACK; // disable ACK
	I2C->SR1; // clear ADDR bit
	I2C->SR3;
    while (I2C->SR1 & I2C_SR1_ADDR);
	I2C->CR2 |= I2C_CR2_STOP;
	while(!(I2C->SR1 & I2C_SR1_RXNE));
	rxBuff[0] = I2C->DR;
	return TRUE;
}
