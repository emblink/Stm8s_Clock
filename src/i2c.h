#ifndef __I2C_H
#define __I2C_H

#include "stm8s.h"
/* Choose one of the communication methods */
//#define I2C_INTERRUPT_METHOD
#define I2C_POLLING_METHOD

void i2cInit(void);
bool i2c_send(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t txBuff[], uint16_t txSize);
bool i2c_read(uint8_t deviceAddress, uint8_t deviceRegister, uint8_t rxBuff[], uint16_t rxSize);

#endif // __I2C_H