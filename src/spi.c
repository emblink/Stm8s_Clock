#include "stm8s.h"
#include "spi.h"
#include "gpio.h"

void SPI_DeInit(void)
{
  SPI->CR1    = SPI_CR1_RESET_VALUE;
  SPI->CR2    = SPI_CR2_RESET_VALUE;
  SPI->ICR    = SPI_ICR_RESET_VALUE;
  SPI->SR     = SPI_SR_RESET_VALUE;
  SPI->CRCPR  = SPI_CRCPR_RESET_VALUE;
}
	
void SPI_Init(SPI_FirstBit_TypeDef FirstBit, SPI_BaudRatePrescaler_TypeDef BaudRatePrescaler, 
              SPI_Mode_TypeDef Mode, SPI_ClockPolarity_TypeDef ClockPolarity, 
              SPI_ClockPhase_TypeDef ClockPhase, SPI_DataDirection_TypeDef Data_Direction, 
              SPI_NSS_TypeDef Slave_Management, uint8_t CRCPolynomial)
{  
  /* Frame Format, BaudRate, Clock Polarity and Phase configuration */
  SPI->CR1 = (uint8_t)((uint8_t)((uint8_t)FirstBit | BaudRatePrescaler) |
                       (uint8_t)((uint8_t)ClockPolarity | ClockPhase));
  
  /* Data direction configuration: BDM, BDOE and RXONLY bits */
  SPI->CR2 = (uint8_t)((uint8_t)(Data_Direction) | (uint8_t)(Slave_Management));
  
  if (Mode == SPI_MODE_MASTER)
  {
    SPI->CR2 |= (uint8_t)SPI_CR2_SSI;
  }
  else
  {
    SPI->CR2 &= (uint8_t)~(SPI_CR2_SSI);
  }
  
  /* Master/Slave mode configuration */
  SPI->CR1 |= (uint8_t)(Mode);
  
  /* CRC configuration */
  SPI->CRCPR = (uint8_t)CRCPolynomial;
}
	
void SPI_CalculateCRCCmd(FunctionalState NewState)
{
  /* Check function parameters */
  assert_param(IS_FUNCTIONALSTATE_OK(NewState));
  
  if (NewState != DISABLE)
  {
    SPI->CR2 |= SPI_CR2_CRCEN; /* Enable the CRC calculation*/
  }
  else
  {
    SPI->CR2 &= (uint8_t)(~SPI_CR2_CRCEN); /* Disable the CRC calculation*/
  }
}

void SPI_Cmd(FunctionalState NewState)
{
  /* Check function parameters */
  assert_param(IS_FUNCTIONALSTATE_OK(NewState));
  
  if (NewState != DISABLE)
  {
    SPI->CR1 |= SPI_CR1_SPE; /* Enable the SPI peripheral*/
  }
  else
  {
    SPI->CR1 &= (uint8_t)(~SPI_CR1_SPE); /* Disable the SPI peripheral*/
  }
}    
    
void spiPushByte(uint8_t byte)
{
	SPI->DR = byte;
    while (!(SPI->SR & SPI_FLAG_TXE));
}

void spiPushData(void)
{
	gpioWritePin(SPI_CS_PIN, FALSE);
}

void spiLatchData(void)
{
	gpioWritePin(SPI_CS_PIN, TRUE);
}