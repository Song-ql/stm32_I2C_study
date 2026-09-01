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
#include "master.h"
#include "slave.h"
#include "BSP_IIC.h"
#include <string.h>
#include "BSP_UART.h"
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

/* USER CODE END Variables */
osThreadId MasterTaskHandle;
osThreadId SlaveTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Master_Task(void const * argument);
void Slave_Task(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

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
  /* definition and creation of MasterTask */
  osThreadDef(MasterTask, Master_Task, osPriorityNormal, 0, 128);
  MasterTaskHandle = osThreadCreate(osThread(MasterTask), NULL);

  /* definition and creation of SlaveTask */
  osThreadDef(SlaveTask, Slave_Task, osPriorityNormal, 0, 128);
  SlaveTaskHandle = osThreadCreate(osThread(SlaveTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_Master_Task */
/**
  * @brief  Function implementing the MasterTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Master_Task */
void Master_Task(void const * argument)
{
  /* USER CODE BEGIN Master_Task */
	
  /* Infinite loop: 周期性发送时间到从机, 并读取从机传感器数据 */
  for(;;)
  {
    /* 主机发送时间到从机 */
    if (Master_SendTime(&master_time) == 0)
    {
      printf("[Master] Send Time: %04u-%02u-%02u %02u:%02u:%02u\r\n",
             master_time.year, master_time.month, master_time.day,
             master_time.hour, master_time.minute, master_time.second);
    }
    else
    {
      printf("[Master] Send Time FAILED\r\n");
    }

    osDelay(500);

    /* 主机读取从机温湿度、光照强度 (阻塞接收方式) */
    if (Master_ReadSensor() == 0)
    {
      /* 解析传感器帧 */
      if (Master_ParseSensor(g_master_buffer.rx_buf, &g_master_sensor) == 0)
      {
        printf("[Master] Read Sensor: Temp=%d.%d C, Humi=%d.%d %%, Light=%d lux\r\n",
                g_master_sensor.temperature / 10, g_master_sensor.temperature % 10,
                g_master_sensor.humidity / 10, g_master_sensor.humidity % 10,
                g_master_sensor.light);
      }
      else
      {
        printf("[Master] Sensor PARSE ERROR, dropped\r\n");
      }
    }
    else
    {
      printf("[Master] Read Sensor FAILED\r\n");
    }

    osDelay(500);
  }
  /* USER CODE END Master_Task */
}

/* USER CODE BEGIN Header_Slave_Task */
/**
* @brief Function implementing the SlaveTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Slave_Task */
void Slave_Task(void const * argument)
{
  /* USER CODE BEGIN Slave_Task */

  /* Infinite loop: 阻塞式收发循环
   * 协议顺序与主机匹配: 主机写时间 -> 从机 ReceiveTime
   *                     主机读传感器 -> 从机 TransmitSensor
   */
  for(;;)
  {
    /* 1. 阻塞等待主机写入时间数据 */
    if (Slave_ReceiveTime() == 0)
    {
      Slave_ParseTime(g_slave_buffer.rx_buf, &g_slave_time);
      printf("[Slave]  Recv Time: %04u-%02u-%02u %02u:%02u:%02u\r\n",
             g_slave_time.year, g_slave_time.month, g_slave_time.day,
             g_slave_time.hour, g_slave_time.minute, g_slave_time.second);
    }

    /* 2. 更新传感器数据并打包到 tx_buf (供后续发送) */
    Slave_UpdateSensor();

    /* 3. 阻塞等待主机读取传感器数据 */
    Slave_TransmitSensor();
  }
  /* USER CODE END Slave_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

