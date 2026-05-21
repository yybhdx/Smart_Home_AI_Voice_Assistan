#include "hc-sr501.h"
#include "gpio.h"
#include "usart.h"
#include "myoled.h"

extern uint8_t buzzer_bit2;

uint8_t hc_sr501_value = 0;

void hc_sr501_task(void)
{
  // 由于 main 循环中有 HAL_Delay(1000)，采样频率较低
  // 采样即确认，以保证响应灵敏度
  uint8_t current_pin_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
	
  if(current_pin_state == 1)
  {
    hc_sr501_value = 1;
    buzzer_bit2 = 1;
  }
  else
  {
    hc_sr501_value = 0;
    buzzer_bit2 = 0;
  }
  
  // 调试输出
  // my_printf(&huart1, "hc_sr501_value = %d\r\n", hc_sr501_value);
}

