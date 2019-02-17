#include "stm8s.h"
#include "i2c.h"
#include "stm8s_i2c.h"

#define NULL 0
#define STM8S_I2C_ADDRESS 0xFF

typedef enum {
  /* SR1 register flags */
  I2C_SR1_BIT_TXEMPTY             = 7,  /*!< Transmit Data Register Empty flag */
  I2C_SR1_BIT_RXNOTEMPTY          = 6,  /*!< Read Data Register Not Empty flag */
  I2C_SR1_BIT_STOPDETECTION       = 4,  /*!< Stop detected flag */
  I2C_SR1_BIT_HEADERSENT          = 3,  /*!< 10-bit Header sent flag */
  I2C_SR1_BIT_TRANSFERFINISHED    = 2,  /*!< Data Byte Transfer Finished flag */
  I2C_SR1_BIT_ADDRESSSENTMATCHED  = 1,  /*!< Address Sent/Matched (master/slave) flag */
  I2C_SR1_BIT_STARTDETECTION      = 0,  /*!< Start bit sent flag */

  /* SR2 register flags */
  I2C_SR2_BIT_WAKEUPFROMHALT      = 5,  /*!< Wake Up From Halt Flag */
  I2C_SR2_BIT_OVERRUNUNDERRUN     = 3,  /*!< Overrun/Underrun flag */
  I2C_SR2_BIT_ACKNOWLEDGEFAILURE  = 2,  /*!< Acknowledge Failure Flag */
  I2C_SR2_BIT_ARBITRATIONLOSS     = 1,  /*!< Arbitration Loss Flag */
  I2C_SR2_BIT_BUSERROR            = 0,  /*!< Misplaced Start or Stop condition */

  /* SR3 register flags */
  I2C_SR3_BIT_GENERALCALL         = 4,  /*!< General Call header received Flag */
  I2C_SR3_BIT_TRANSMITTERRECEIVER = 2,  /*!< Transmitter Receiver Flag */
  I2C_SR3_BIT_BUSBUSY             = 1,  /*!< Bus Busy Flag */
  I2C_SR3_BIT_MASTERSLAVE         = 0   /*!< Master Slave Flag */
} I2cRegisterBits;

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
	if(busyBus || !txBuff || !txSize)
		return FALSE;
	busyBus = TRUE;
	busAddress = deviceAddress << 1;
	memoryAdress = deviceRegister;
	buffPtr = txBuff;
	dataLength = txSize;
	buffIdx = 0;
	I2C_GenerateSTART(ENABLE);
	return TRUE;
}

bool i2c_read(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t rxBuff[], uint16_t rxSize)
{
	if(I2C->SR3 & I2C_SR3_BIT_BUSBUSY|| !rxBuff || !rxSize) // I2C_SR3_BUSY
		return FALSE;
	busyBus = TRUE;
	busAddress = deviceAddress << 1 | 0x01;
	memoryAdress = deviceRegister;
	buffPtr = rxBuff;
	dataLength = rxSize;
	buffIdx = 0;
	I2C_GenerateSTART(ENABLE);
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
	
	while(I2C->SR3 & (1 << I2C_SR3_BIT_BUSBUSY)); //wait for previous transaction finish
	
	I2C->CR2 |= I2C_CR2_START;  // generate start
	while (!(I2C->SR1 & (1 << I2C_SR1_BIT_STARTDETECTION)));
	I2C->SR1; // clear SB bit
	I2C->DR = deviceAddress << 1;
	while (!(I2C->SR1 & (1 << I2C_SR1_BIT_ADDRESSSENTMATCHED)));
	I2C->SR1; // clear ADDR bit
	I2C->SR3;
	while (!(I2C->SR1 & (1 << I2C_SR1_BIT_TXEMPTY)));
	I2C->DR = deviceRegister;
	uint16_t i = 0;
	while (i < txSize) {
		while(!(I2C->SR1 & (1 << I2C_SR1_BIT_TXEMPTY))); // TXE=1, shift register empty, data register empty, write DR register.
		I2C->DR = txBuff[i++];
	}
	while(!(I2C->SR1 & (1 << I2C_SR1_BIT_TXEMPTY)) && !(I2C->SR1 & (1 << I2C_SR1_BIT_TRANSFERFINISHED)));
	I2C->CR2 |= I2C_CR2_STOP;
	return TRUE;
}

bool i2c_read(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t rxBuff[], uint16_t rxSize)
{
	if(!rxBuff || !rxSize)
		return FALSE;
	
	while(I2C->SR3 & (1 << I2C_SR3_BIT_BUSBUSY)); //wait for previous transaction finish
	
	return TRUE;
}
#endif // I2C_POLLING_METHOD


