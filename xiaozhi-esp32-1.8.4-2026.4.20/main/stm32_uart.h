#ifndef STM32_UART_H
#define STM32_UART_H

#ifdef __cplusplus
extern "C" {
#endif

// FreeRTOS task: waits for WiFi, starts Huawei MQTT, reads STM32 data and publishes
void stm32_uart_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif
