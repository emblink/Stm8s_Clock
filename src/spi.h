#ifndef __SPI_H
#define __SPI_H

#include "stm8s.h"

typedef enum {
  SPI_FIRSTBIT_MSB = (uint8_t)0x00, /*!< MSB bit will be transmitted first */
  SPI_FIRSTBIT_LSB = (uint8_t)0x80  /*!< LSB bit will be transmitted first */
} SPI_FirstBit_TypeDef;

typedef enum {
  SPI_BAUDRATEPRESCALER_2   = (uint8_t)0x00, /*!< SPI frequency = frequency(CPU)/2 */
  SPI_BAUDRATEPRESCALER_4   = (uint8_t)0x08, /*!< SPI frequency = frequency(CPU)/4 */
  SPI_BAUDRATEPRESCALER_8   = (uint8_t)0x10, /*!< SPI frequency = frequency(CPU)/8 */
  SPI_BAUDRATEPRESCALER_16  = (uint8_t)0x18, /*!< SPI frequency = frequency(CPU)/16 */
  SPI_BAUDRATEPRESCALER_32  = (uint8_t)0x20, /*!< SPI frequency = frequency(CPU)/32 */
  SPI_BAUDRATEPRESCALER_64  = (uint8_t)0x28, /*!< SPI frequency = frequency(CPU)/64 */
  SPI_BAUDRATEPRESCALER_128 = (uint8_t)0x30, /*!< SPI frequency = frequency(CPU)/128 */
  SPI_BAUDRATEPRESCALER_256 = (uint8_t)0x38  /*!< SPI frequency = frequency(CPU)/256 */
} SPI_BaudRatePrescaler_TypeDef;

typedef enum {
  SPI_FLAG_BSY    = (uint8_t)0x80, /*!< Busy flag */
  SPI_FLAG_OVR    = (uint8_t)0x40, /*!< Overrun flag */
  SPI_FLAG_MODF   = (uint8_t)0x20, /*!< Mode fault */
  SPI_FLAG_CRCERR = (uint8_t)0x10, /*!< CRC error flag */
  SPI_FLAG_WKUP   = (uint8_t)0x08, /*!< Wake-up flag */
  SPI_FLAG_TXE    = (uint8_t)0x02, /*!< Transmit buffer empty */
  SPI_FLAG_RXNE   = (uint8_t)0x01  /*!< Receive buffer empty */
} SPI_Flag_TypeDef;

typedef enum {
  SPI_MODE_MASTER = (uint8_t)0x04, /*!< SPI Master configuration */
  SPI_MODE_SLAVE  = (uint8_t)0x00  /*!< SPI Slave configuration */
} SPI_Mode_TypeDef;

typedef enum {
  SPI_CLOCKPOLARITY_LOW  = (uint8_t)0x00, /*!< Clock to 0 when idle */
  SPI_CLOCKPOLARITY_HIGH = (uint8_t)0x02  /*!< Clock to 1 when idle */
} SPI_ClockPolarity_TypeDef;

typedef enum {
  SPI_CLOCKPHASE_1EDGE = (uint8_t)0x00, /*!< The first clock transition is the first data capture edge */
  SPI_CLOCKPHASE_2EDGE = (uint8_t)0x01  /*!< The second clock transition is the first data capture edge */
} SPI_ClockPhase_TypeDef;

typedef enum {
  SPI_DATADIRECTION_2LINES_FULLDUPLEX = (uint8_t)0x00, /*!< 2-line uni-directional data mode enable */
  SPI_DATADIRECTION_2LINES_RXONLY     = (uint8_t)0x04, /*!< Receiver only in 2 line uni-directional data mode */
  SPI_DATADIRECTION_1LINE_RX          = (uint8_t)0x80, /*!< Receiver only in 1 line bi-directional data mode */
  SPI_DATADIRECTION_1LINE_TX          = (uint8_t)0xC0  /*!< Transmit only in 1 line bi-directional data mode */
} SPI_DataDirection_TypeDef;

typedef enum
{
  SPI_NSS_SOFT  = (uint8_t)0x02, /*!< Software slave management disabled */
  SPI_NSS_HARD  = (uint8_t)0x00  /*!< Software slave management enabled */
} SPI_NSS_TypeDef;

void SPI_Init(SPI_FirstBit_TypeDef FirstBit, SPI_BaudRatePrescaler_TypeDef BaudRatePrescaler, 
              SPI_Mode_TypeDef Mode, SPI_ClockPolarity_TypeDef ClockPolarity, 
              SPI_ClockPhase_TypeDef ClockPhase, SPI_DataDirection_TypeDef Data_Direction, 
              SPI_NSS_TypeDef Slave_Management, uint8_t CRCPolynomial);

void SPI_DeInit(void);
void SPI_CalculateCRCCmd(FunctionalState NewState);
void SPI_Cmd(FunctionalState NewState);  
void spiPushByte(uint8_t byte);
void spiPushData(void);
void spiLatchData(void);

#endif // __SPI_H