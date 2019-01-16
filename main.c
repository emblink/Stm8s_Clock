#include "main.h"
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_exti.h"
#include "stm8s_it.h"
#include "stm8s_tim1.h"
#include "stm8s_clk.h"

void delayMs(uint16_t delay)
{
  while(delay--)
    for (uint16_t i = 0; i < 2000; i++);
}

int main( void )
{
  CLK_HSICmd(ENABLE);
  while (!CLK->ICKR & (1 << 1)); // wait till stable
  CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1); // 16MHz SYS
  CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1); // 16MHz CPU
  
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, ENABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, ENABLE);
  
  GPIO_Init(GPIOC, RED_LED_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(GPIOD, GREEN_LED_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(GPIOC, BUTTON_PIN, GPIO_MODE_IN_FL_IT);
  EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_RISE_FALL);

  TIM1_DeInit();
  TIM1_PrescalerConfig(15, TIM1_PSCRELOADMODE_UPDATE); // 1599 + 1 = 1600
  TIM1_SetAutoreload(1000); // frequency = 16M / 16 / 1000 = 1000 Hz
  TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
  TIM1_CounterModeConfig(TIM1_COUNTERMODE_UP);
  TIM1_Cmd(ENABLE);
  
  enableInterrupts();
  
  while(1)
  {
    GPIO_WriteReverse(GPIOD, GREEN_LED_PIN);
    delayMs(1000);
  }
  return 0;
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while (1)
  {
  }
}
#endif