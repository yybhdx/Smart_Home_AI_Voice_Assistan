/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "mydht11.h"

#include "myoled.h"

#include "mymq-7.h"

#include "hc-sr501.h"

#include "buzzer.h"

#include "esp32-s3.h"
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
osThreadId_t dht11_Handle;
const osThreadAttr_t dht11_attributes = {
  .name = "dht11",
  .stack_size = 1024, // ï¿½ß³ï¿½Õ»ï¿½ï¿½Ð¡Îª 128 ï¿½ï¿½ï¿½ï¿½ (128 * 4 ï¿½Ö½ï¿½)
  .priority = (osPriority_t) osPriorityNormal, // ï¿½ß³Ìµï¿½ï¿½ï¿½ï¿½È¼ï¿½Îª osPriorityNormal
};

osThreadId_t OLED_Handle;
const osThreadAttr_t OLED_attributes = {
  .name = "OLED",
  .stack_size = 1024, // ï¿½ß³ï¿½Õ»ï¿½ï¿½Ð¡Îª 128 ï¿½ï¿½ï¿½ï¿½ (128 * 4 ï¿½Ö½ï¿½)
  .priority = (osPriority_t) osPriorityNormal, // ï¿½ß³Ìµï¿½ï¿½ï¿½ï¿½È¼ï¿½Îª osPriorityNormal
};

osThreadId_t MQ7_Handle;
const osThreadAttr_t MQ7_attributes = {
  .name = "MQ7",
  .stack_size = 1024, // ï¿½ß³ï¿½Õ»ï¿½ï¿½Ð¡Îª 128 ï¿½ï¿½ï¿½ï¿½ (128 * 4 ï¿½Ö½ï¿½)
  .priority = (osPriority_t) osPriorityNormal, // ï¿½ß³Ìµï¿½ï¿½ï¿½ï¿½È¼ï¿½Îª osPriorityNormal
};

osThreadId_t HC_SR_501_Handle;
const osThreadAttr_t HC_SR_501_attributes = {
  .name = "HC_SR_501",
  .stack_size = 1024, // ï¿½ß³ï¿½Õ»ï¿½ï¿½Ð¡Îª 128 ï¿½ï¿½ï¿½ï¿½ (128 * 4 ï¿½Ö½ï¿½)
  .priority = (osPriority_t) osPriorityAboveNormal,
};

osThreadId_t buzzer_Handle;
const osThreadAttr_t buzzer_attributes = {
  .name = "buzzer",
  .stack_size = 1024, // 512²»Ïì£¬1024Ïì£¬¿ÉÄÜÊÇÒòÎª·äÃùÆ÷¿ØÖÆÂß¼­ÖÐÓÐ½Ï¶àµÄ²Ù×÷£¬µ¼ÖÂÉÏÏÂÎÄÇÐ»»ºÍµ÷¶È¿ªÏú½Ï´ó£¬Ôö¼ÓÁËÈÎÎñµÄÖ´ÐÐÊ±¼ä£¬´Ó¶øÐèÒª¸ü´óµÄÕ»¿Õ¼äÀ´´æ´¢ÈÎÎñµÄÉÏÏÂÎÄºÍ¾Ö²¿±äÁ¿¡£
  .priority = (osPriority_t) osPriorityAboveNormal,
};

osThreadId_t esp_report_Handle;
const osThreadAttr_t esp_report_attributes = {
  .name = "esp_report",
  .stack_size = 1024, // ï¿½ß³ï¿½Õ»ï¿½ï¿½Ð¡Îª 128 ï¿½ï¿½ï¿½ï¿½ (128 * 4 ï¿½Ö½ï¿½)
  .priority = (osPriority_t) osPriorityAboveNormal, // ï¿½ß³Ìµï¿½ï¿½ï¿½ï¿½È¼ï¿½Îª osPriorityNormal
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024,
  .priority = (osPriority_t) osPriorityNormal,
};


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
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
  /* add threads, ... */
  dht11_Handle = osThreadNew(dht11_task, NULL, &dht11_attributes);

  OLED_Handle = osThreadNew(oled_task, NULL, &OLED_attributes);

  MQ7_Handle = osThreadNew(mq7_task, NULL, &MQ7_attributes);

  HC_SR_501_Handle = osThreadNew(hc_sr501_task, NULL, &HC_SR_501_attributes);

  buzzer_Handle = osThreadNew(Buzzer_Task, NULL, &buzzer_attributes);

  esp_report_Handle = osThreadNew(esp_report, NULL, &esp_report_attributes);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

