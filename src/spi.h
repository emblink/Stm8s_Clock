#ifndef __SPI_H
#define __SPI_H

void spiInit(void);
void spiEnable(void);
void spiDisable(void);
void spiPushByte(uint8_t byte);
void spiSendData(const uint8_t data[], uint16_t len);

#endif // __SPI_H