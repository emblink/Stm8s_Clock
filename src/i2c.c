#include "stm8s.h"
#include "i2c.h"
#include "stm8s_i2c.h"

#define NULL 0
#define STM8S_I2C_ADDRESS 0xFF

void i2cInit(void)
{
  	I2C_DeInit();
	I2C_Init(I2C_MAX_FAST_FREQ, STM8S_I2C_ADDRESS, I2C_DUTYCYCLE_2, I2C_ACK_CURR,
			 I2C_ADDMODE_7BIT, I2C_MAX_INPUT_FREQ);
#ifdef I2C_INTERRUPT_METHOD
	I2C_ITConfig(I2C_IT_EVT | I2C_IT_BUF, ENABLE);
#else
	I2C->ITR &= (~I2C_IT_ERR | ~I2C_IT_EVT | ~I2C_IT_BUF); // disable interrupts
#endif
	I2C_Cmd(ENABLE);
	/* TODO: 21.4.2 I2C master mode - read 293 page of the datasheet
	and add i2c functionality for DS1307 */
}

#ifdef I2C_INTERRUPT_METHOD
static uint8_t busAddress = 0;
static uint8_t memoryAdress = 0;
static uint16_t dataLength = 0;
static uint8_t *buffPtr = NULL;
static uint16_t buffIdx = 0;
static bool busyBus = FALSE;

bool i2c_send(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t txBuff[], uint16_t txSize)
{
	if(I2C->SR3 & I2C_SR3_BUSY || !txBuff || !txSize)
		return FALSE;
	busyBus = TRUE;
	busAddress = deviceAddress << 1;
	memoryAdress = deviceRegister;
	buffPtr = txBuff;
	dataLength = txSize;
	buffIdx = 0;
	I2C->CR2 |= I2C_CR2_START;
	return TRUE;
}

bool i2c_read(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t rxBuff[], uint16_t rxSize)
{
	if(I2C->SR3 & I2C_SR3_BUSY || !rxBuff || !rxSize)
		return FALSE;
	busyBus = TRUE;
	busAddress = (deviceAddress << 1) | 0x01;
	memoryAdress = deviceRegister;
	buffPtr = rxBuff;
	dataLength = rxSize;
	buffIdx = 0;
	I2C->CR2 |= I2C_CR2_START;
	return TRUE;
}

/**
  * @brief I2C Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(I2C_IRQHandler, 19)
{ 	
	I2C_Event_TypeDef event = I2C_GetLastEvent();
	switch(event) {
	case I2C_EVENT_MASTER_MODE_SELECT: // EV5
		I2C->SR1;
		I2C->DR = busAddress;
		break;
	case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED: // EV6
		I2C->SR1;
		I2C->SR3;
		break;
	case I2C_EVENT_MASTER_BYTE_TRANSMITTING: // EV8
		if (buffIdx < dataLength)
			I2C->DR = buffPtr[buffIdx++];
		break;
	case I2C_EVENT_MASTER_BYTE_TRANSMITTED: // EV8_2
		if (buffIdx < dataLength)
			I2C->DR = buffPtr[buffIdx++];
		else {
			I2C_GenerateSTOP(ENABLE);
			busyBus = FALSE;
		}
		break;
	case I2C_EVENT_MASTER_BYTE_RECEIVED:
		break;
	default:
		asm ("nop");
		break;
	}
}
#endif // I2C_INTERRUPT_METHOD

#ifdef I2C_POLLING_METHOD
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

#define I2C_DR_DR        ((uint8_t)0xFF)  /*!< Data Register */

#define I2C_SR1_TXE      ((uint8_t)0x80)  /*!< Data Register Empty (transmitters) */
#define I2C_SR1_RXNE     ((uint8_t)0x40)  /*!< Data Register not Empty (receivers) */
#define I2C_SR1_STOPF    ((uint8_t)0x10)  /*!< Stop detection (Slave mode) */
#define I2C_SR1_ADD10    ((uint8_t)0x08)  /*!< 10-bit header sent (Master mode) */
#define I2C_SR1_BTF      ((uint8_t)0x04)  /*!< Byte Transfer Finished */
#define I2C_SR1_ADDR     ((uint8_t)0x02)  /*!< Address sent (master mode)/matched (slave mode) */
#define I2C_SR1_SB       ((uint8_t)0x01)  /*!< Start Bit (Master mode) */

#define I2C_SR2_WUFH     ((uint8_t)0x20)  /*!< Wake-up from Halt */
#define I2C_SR2_OVR      ((uint8_t)0x08)  /*!< Overrun/Underrun */
#define I2C_SR2_AF       ((uint8_t)0x04)  /*!< Acknowledge Failure */
#define I2C_SR2_ARLO     ((uint8_t)0x02)  /*!< Arbitration Lost (master mode) */
#define I2C_SR2_BERR     ((uint8_t)0x01)  /*!< Bus Error */

#define I2C_SR3_GENCALL  ((uint8_t)0x10)  /*!< General Call Header (Slave mode) */
#define I2C_SR3_TRA      ((uint8_t)0x04)  /*!< Transmitter/Receiver */
#define I2C_SR3_BUSY     ((uint8_t)0x02)  /*!< Bus Busy */
#define I2C_SR3_MSL      ((uint8_t)0x01)  /*!< Master/Slave */

#define I2C_CR2_SWRST ((uint8_t)0x80)     /*!< Software Reset */
#define I2C_CR2_POS   ((uint8_t)0x08)     /*!< Acknowledge */
#define I2C_CR2_ACK   ((uint8_t)0x04)     /*!< Acknowledge Enable */
#define I2C_CR2_STOP  ((uint8_t)0x02)     /*!< Stop Generation */
#define I2C_CR2_START ((uint8_t)0x01)     /*!< Start Generation */

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
	while (!(I2C->SR1 & I2C_SR1_TXE));
	I2C->DR = deviceRegister;
	while (!(I2C->SR1 & I2C_SR1_TXE));
	I2C->CR2 |= I2C_CR2_START;
	while (!(I2C->SR1 & I2C_SR1_SB));
	I2C->DR = (deviceAddress << 1) | 0x01;
	while (!(I2C->SR1 & I2C_SR1_ADDR));
	I2C->CR2 &= ~I2C_CR2_ACK; // disable ACK
	I2C->SR1; // clear ADDR bit
	I2C->SR3;
	I2C->CR2 |= I2C_CR2_STOP;
	while(!(I2C->SR1 & I2C_SR1_RXNE));
	rxBuff[0] = I2C->DR;
	return TRUE;
}
#endif // I2C_POLLING_METHOD


