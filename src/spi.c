#include "stm8s.h"
#include "spi.h"

void spiInit(void)
{
    SPI->CR1   = 0x00;
    SPI->CR2   = 0x00;
    SPI->ICR   = 0x00;
    SPI->SR    = 0x02;
    SPI->CRCPR = 0x07;

    /* Frame Format, BaudRate, Clock Polarity and Phase configuration */
    SPI->CR1 &= ~SPI_CR1_LSBFIRST; // SPI_FIRSTBIT_MSB
    SPI->CR1 &= ~SPI_CR1_BR; // SPI_BAUDRATEPRESCALER_2
    SPI->CR1 |= SPI_CR1_MSTR; // SPI_MODE_MASTER
    SPI->CR1 &= ~SPI_CR1_CPOL; // SCK to 0 when idle
    SPI->CR1 &= ~SPI_CR1_CPHA; // 0: The first clock transition is the first data capture edge   // TODO: change config and observe display stability

    SPI->CR2 |= SPI_CR2_BDM; // 1-line bidirectional data mode selected
    SPI->CR2 |= SPI_CR2_BDOE; // Output enabled (transmit-only mode)
    SPI->CR2 &= ~SPI_CR2_CRCEN; // CRC calculation disabled
    SPI->CR2 |= SPI_CR2_SSM; // Software slave management enabled
    SPI->CR2 |= SPI_CR2_SSI; // mode master, or MODF error

    SPI->CRCPR = 0xA5;
}

void spiEnable(void)
{
    SPI->CR1 |= SPI_CR1_SPE; /* Enable the SPI peripheral*/
}

void spiDisable(void)
{
    SPI->CR1 &= ~SPI_CR1_SPE; /* Disable the SPI peripheral*/
}

void spiPushByte(uint8_t byte)
{
    while ((SPI->SR &= SPI_SR_TXE) == RESET);
        SPI->DR = byte;
}

void spiSendData(const uint8_t data[], uint16_t len)
{
    GPIOC->ODR &= ~(1 << 7); // #define SPI_CS_PIN GPIO_PIN_7
    for (uint16_t i = 0; i < len; i++)
        spiPushByte(data[i]);
    GPIOC->ODR |= 1 << 7;
}
