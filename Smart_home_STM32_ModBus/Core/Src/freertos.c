/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : FreeRTOS 任务及系统初始化代码
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include "myoled.h"
#include "mymq-7.h"
#include "hc-sr501.h"
#include "buzzer.h"
#include "esp32-s3.h"
#include "temp_humi.h"
#include "modbus.h"
#include "crc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// OLED 显示任务句柄及属性
osThreadId_t OLED_Handle;
const osThreadAttr_t OLED_attributes = {
    .name = "OLED",
    .stack_size = 1024,                         // 线程栈大小 (单位：字节)
    .priority = (osPriority_t)osPriorityNormal, // 优先级：普通
};

// RS485 ModBus 温湿度读取任务句柄及属性
osThreadId_t RS485_ModBus_humi_temp_Handle;
const osThreadAttr_t RS485_ModBus_humi_temp_attributes = {
    .name = "RS485_ModBus_humi_temp",
    .stack_size = 2048,                               // 线程栈大小
    .priority = (osPriority_t)osPriorityAboveNormal,  // 优先级：高于普通
};

// MQ-7 烟雾/CO 传感器任务句柄及属性
osThreadId_t MQ7_Handle;
const osThreadAttr_t MQ7_attributes = {
    .name = "MQ7",
    .stack_size = 1024,
    .priority = (osPriority_t)osPriorityNormal,
};

// HC-SR501 人体红外感应任务句柄及属性
osThreadId_t HC_SR_501_Handle;
const osThreadAttr_t HC_SR_501_attributes = {
    .name = "HC_SR_501",
    .stack_size = 1024,
    .priority = (osPriority_t)osPriorityAboveNormal,
};

// 蜂鸣器控制任务句柄及属性
osThreadId_t buzzer_Handle;
const osThreadAttr_t buzzer_attributes = {
    .name = "buzzer",
    .stack_size = 1024,
    .priority = (osPriority_t)osPriorityAboveNormal,
};

// ESP32-S3 数据上报任务句柄及属性
osThreadId_t esp_report_Handle;
const osThreadAttr_t esp_report_attributes = {
    .name = "esp_report",
    .stack_size = 1024,
    .priority = (osPriority_t)osPriorityAboveNormal,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS 初始化
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  
  // 创建 OLED 刷新任务
  OLED_Handle = osThreadNew(oled_task, NULL, &OLED_attributes);
  
  // 创建温湿度读取任务 (ModBus RTU)
  RS485_ModBus_humi_temp_Handle = osThreadNew(Temp_Humi_Read, NULL, &RS485_ModBus_humi_temp_attributes);

  // 创建 MQ-7 采集任务
  MQ7_Handle = osThreadNew(mq7_task, NULL, &MQ7_attributes);

  // 创建人体感应采集任务
  HC_SR_501_Handle = osThreadNew(hc_sr501_task, NULL, &HC_SR_501_attributes);

  // 创建蜂鸣器控制任务
  buzzer_Handle = osThreadNew(Buzzer_Task, NULL, &buzzer_attributes);

  // 创建 ESP32 数据上报任务
  esp_report_Handle = osThreadNew(esp_report, NULL, &esp_report_attributes);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  默认任务（心跳灯）
 * @param  argument: 不使用
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // 翻转 PC13 (板载 LED)
    osDelay(500); // 延时 500ms
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
