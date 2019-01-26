#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_spi.h"
#include "main.h"
#include "max7219.h"

typedef enum Max7219Register {
    MAX7219_NOOP_REG = 0x00,
    MAX7219_ROW_1_REG = 0x01,
    MAX7219_ROW_2_REG = 0x02,
    MAX7219_ROW_3_REG = 0x03,
    MAX7219_ROW_4_REG = 0x04,
    MAX7219_ROW_5_REG = 0x05,
    MAX7219_ROW_6_REG = 0x06,
    MAX7219_ROW_7_REG = 0x07,
    MAX7219_ROW_8_REG = 0x08,
    MAX7219_DECODE_MODE_REG = 0x9,
    MAX7219_INTENSITY_REG = 0xA,
    MAX7219_SCAN_LIMIT_REG = 0xB,
    MAX7219_SHUTDOWN_REG = 0xC,
    MAX7219_DISPLAY_TEST_REG = 0xF
} Max7219Register;

static void max7219SendCommand(uint8_t panleIndex, 
                               Max7219Register max7219Register, 
                               uint8_t commandData);

static void max7219SendCommandToAll(Max7219Register max7219Register, 
                                    uint8_t commandData);

static void max7219FillRow(uint8_t panelNumber, Max7219Register max7219Register, uint8_t symbolByte, uint8_t rowBuffer[]);
static void max7219SendRow(Max7219Register max7219RowRegister, const uint8_t rowData[]);

void max7219Init(void)
{
  max7219SendCommandToAll(MAX7219_SHUTDOWN_REG, 0x01); // Turn On. Normal Operation
  max7219SendCommandToAll(MAX7219_SCAN_LIMIT_REG, 0x07); // Activate all rows.
  max7219SendCommandToAll(MAX7219_INTENSITY_REG, 0x01); // 0 - 15 levels of Intensity.
  max7219SendCommandToAll(MAX7219_DECODE_MODE_REG, 0x00); // No decode mode.
  max7219SendCommandToAll(MAX7219_DISPLAY_TEST_REG, 0x00); // Display-Test off.
  max7219ClearAllPanels();
}

void max7219SetTestMode(uint8_t panleIndex, bool testEnable)
{
  max7219SendCommand(panleIndex, MAX7219_DISPLAY_TEST_REG, testEnable);
}

void max7219SetPanelState(uint8_t panleIndex, bool panelEnable)
{
  max7219SendCommand(panleIndex, MAX7219_SHUTDOWN_REG, panelEnable);
}

void max7219SetIntensity(uint8_t panleIndex, uint8_t intensity)
{
  max7219SendCommand(panleIndex, MAX7219_INTENSITY_REG, intensity);
}

static void max7219SendCommand(uint8_t panelIndex, 
                               Max7219Register max7219Register, 
                               uint8_t commandData)
{ 
  uint8_t dataBuff[PANEL_COUNT * 2];
  max7219FillRow(panelIndex, max7219Register, commandData, dataBuff);
  max7219SendRow(max7219Register, dataBuff);
}

/*
inline static void spiSendByte(uint8_t byte)
{
  while (SPI_GetFlagStatus(SPI_FLAG_TXE) == RESET);
  SPI->DR = byte;
}
*/

void max7219SendCommandToAll(Max7219Register max7219Register, uint8_t commandData)
{
  GPIOC->ODR &= ~SPI_CS_PIN;
  for (uint8_t i = 0; i < PANEL_COUNT; i++) {
    SPI->DR = max7219Register;
    while (!(SPI->SR & SPI_FLAG_TXE));
    SPI->DR = commandData;
    while (!(SPI->SR & SPI_FLAG_TXE));
  }
  GPIOC->ODR |= SPI_CS_PIN;
}

static void max7219SendRow(Max7219Register max7219RowRegister, const uint8_t rowData[PANEL_COUNT * 2])
{
  GPIOC->ODR &= ~SPI_CS_PIN;
  for (uint8_t i = 0; i < PANEL_COUNT * 2; i++) {
    SPI->DR = rowData[i];
    while (!(SPI->SR & SPI_FLAG_TXE));
  }
  GPIOC->ODR |= SPI_CS_PIN;
}

void max7219SendSymbol(uint8_t panelNumber, const uint8_t symbolData[8])
{
  uint8_t rowBuff[PANEL_COUNT * 2];
  for (Max7219Register reg = MAX7219_ROW_1_REG; reg <= MAX7219_ROW_8_REG; reg++) {
    max7219FillRow(panelNumber, reg, symbolData[reg - 1], rowBuff);
    max7219SendRow(reg, rowBuff);
  }
}

static void max7219FillRow(uint8_t panelNumber, Max7219Register max7219Register, uint8_t symbolByte, uint8_t rowBuffer[])
{
  uint8_t buffIdx = 0;
  for (uint8_t i = 0; i < PANEL_COUNT; i++) {
    if (i == PANEL_COUNT - panelNumber - 1) {
      rowBuffer[buffIdx++] = max7219Register;
      rowBuffer[buffIdx++] = symbolByte;
    } else {
      rowBuffer[buffIdx++] = 0x00; // No Operation
      rowBuffer[buffIdx++] = 0x00; // No Operation
    }
  }
}

void max7219ClearAllPanels(void)
{
  const uint8_t clearBuff[PANEL_COUNT * 2] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (Max7219Register reg = MAX7219_ROW_1_REG; reg <= MAX7219_ROW_8_REG; reg++)
    max7219SendRow(reg, clearBuff);
}

void max7219ClearPanel(uint8_t panelNumber)
{
  const uint8_t clearBuff[PANEL_COUNT * 2] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  max7219SendSymbol(panelNumber, clearBuff);
}

/*
function transform array of columns into array of rows, that can be sent to max7219
static void trans_panel(uint8_t *array, uint8_t *fill_array)
{
	for (uint8_t row = 0; row < 8; row++) {
		uint8_t bit = 1 << row;
		for (int8_t i = 7, bit_indx = 0; i >= 0; i--, bit_indx++) {
			uint8_t data = 0x00;
			data = array[i] & bit; // read bit
			data >>= row; // move to 0 position
			data <<= bit_indx; // move to proper position
			if (!data) { // == 0x00
				uint8_t data_and_mask = ~(1 << bit_indx); // create mask
				fill_array[row] &= (data | data_and_mask); // write 0 in bit
			}
			else
				fill_array[row] |= (data); // write 1 in bit
		}
    }
}


void update_screen(uint8_t *screenbuff)
{
	uint8_t buff_to_send[LED_NUM * 8];
	for (uint8_t n = 0; n < LED_NUM; n++) {
		uint8_t indx = n << 3;
		trans_panel(&screenbuff[indx], &buff_to_send[indx]);
		max7219_send_char_to(n, &buff_to_send[indx]);
	}
}

*/