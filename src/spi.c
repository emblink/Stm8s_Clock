
  SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_2, SPI_MODE_MASTER, 
           SPI_CLOCKPOLARITY_HIGH, SPI_CLOCKPHASE_2EDGE, 
           SPI_DATADIRECTION_1LINE_TX, SPI_NSS_SOFT, 0xA5);
  SPI_CalculateCRCCmd(DISABLE);
  SPI_Cmd(ENABLE);
  
  
  
  
void spiSendData(const uint8_t data[], uint16_t len){
    /*!< Wait until the transmit buffer is empty */
    GPIO_WriteLow(GPIOC, SPI_CS_PIN);
    for (uint16_t i = 0; i < len; i++) {
      while (SPI_GetFlagStatus(SPI_FLAG_TXE) == RESET);
      SPI->DR = Data;
    }
    GPIO_WriteHigh(GPIOC, SPI_CS_PIN);
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
