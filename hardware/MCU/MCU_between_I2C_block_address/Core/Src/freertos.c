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
	TimeData_t master_time = {2026, 8, 27, 12, 0, 0};
  /* Infinite loop: 周期性发送时间到从机, 并读取从机传感器数据 */
  for(;;)
  {
    /* 主机带地址发送时间到从机 (写入 SLAVE_REG_TIME 寄存器) */
    if (Master_SendTimeByAddr(&master_time) == 0)
    {
      printf("[Master] Send Time(addr=0x%02X): %04u-%02u-%02u %02u:%02u:%02u\r\n",
             SLAVE_REG_TIME,
             master_time.year, master_time.month, master_time.day,
             master_time.hour, master_time.minute, master_time.second);
    }
    else
    {
      printf("[Master] Send Time FAILED\r\n");
    }

    /* 时间自增 (每秒 +1 秒, 简化处理) */
    master_time.second++;
    if (master_time.second >= 60)
    {
      master_time.second = 0;
      master_time.minute++;
      if (master_time.minute >= 60)
      {
        master_time.minute = 0;
        master_time.hour++;
        if (master_time.hour >= 24)
        {
          master_time.hour = 0;
        }
      }
    }

    osDelay(500);

    /* 主机带地址读取从机温湿度、光照强度
     * 分别从 SLAVE_REG_TEMP / SLAVE_REG_HUMI / SLAVE_REG_LIGHT 三个独立寄存器读取,
     * Master_ReadSensorByAddr 内部已组装到 g_master_sensor。
     */
    if (Master_ReadSensorByAddr() == 0)
    {
      /* 光照合法范围兜底校验 (从机生成范围 0 ~ 999) */
      if ((g_master_sensor.light >= 0) && (g_master_sensor.light <= 999))
      {
        printf("[Master] Read Sensor(addr T=0x%02X H=0x%02X L=0x%02X): Temp=%d.%d C, Humi=%d.%d %%, Light=%d lux\r\n",
               SLAVE_REG_TEMP, SLAVE_REG_HUMI, SLAVE_REG_LIGHT,
               g_master_sensor.temperature / 10, g_master_sensor.temperature % 10,
               g_master_sensor.humidity / 10, g_master_sensor.humidity % 10,
               g_master_sensor.light);
      }
      else
      {
        printf("[Master] Sensor data INVALID, dropped\r\n");
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

  /* Infinite loop: 阻塞式收发循环 (带数据地址)
   * 协议顺序与主机匹配:
   *   主机写时间到寄存器      -> 从机 Slave_ReceiveByAddr
   *   主机依次读 TEMP/HUMI/LIGHT -> 从机 Slave_TransmitByAddr x3
   */
  for(;;)
  {
    /* 1. 阻塞等待主机写入时间数据 (带寄存器地址) */
    if (Slave_ReceiveByAddr() == 0)
    {
      /* Slave_ReceiveByAddr 内部已将寄存器中的时间解析到 g_slave_time */
      printf("[Slave]  Recv Time(addr=0x%02X): %04u-%02u-%02u %02u:%02u:%02u\r\n",
             SLAVE_REG_TIME,
             g_slave_time.year, g_slave_time.month, g_slave_time.day,
             g_slave_time.hour, g_slave_time.minute, g_slave_time.second);
    }

    /* 2. 更新传感器数据并同步到三个独立寄存器 */
    Slave_UpdateSensor();

    /* 3. 阻塞等待主机依次读取温度/湿度/光照寄存器 (各 2 字节) */
    Slave_TransmitByAddr();   /* SLAVE_REG_TEMP  */
    Slave_TransmitByAddr();   /* SLAVE_REG_HUMI  */
    Slave_TransmitByAddr();   /* SLAVE_REG_LIGHT */
  }
  /* USER CODE END Slave_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

