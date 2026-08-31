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

/* I2C 互斥量: 保护恢复路径, 避免一侧恢复期间另一侧发起事务导致连锁恢复 */
osMutexId g_i2c_mutex;

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
  /* 创建 I2C 互斥量: 保护恢复路径, 避免一侧恢复期间另一侧发起事务 */
  osMutexDef(i2c_mutex);
  g_i2c_mutex = osMutexCreate(osMutex(i2c_mutex));
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
  /* Infinite loop: 周期性发送时间到从机, 并单独读取从机传感器数据 */
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

    /* 1. 单独读取温度寄存器 (SLAVE_REG_TEMP, 2 字节) */
    if (Master_ReadTempByAddr() == 0)
    {
      printf("[Master] Read Temp(addr=0x%02X): %d.%d C\r\n",
             SLAVE_REG_TEMP,
             g_master_sensor.temperature / 10, g_master_sensor.temperature % 10);
    }
    else
    {
      printf("[Master] Read Temp FAILED\r\n");
    }

    osDelay(500);

    /* 2. 单独读取湿度寄存器 (SLAVE_REG_HUMI, 2 字节) */
    if (Master_ReadHumiByAddr() == 0)
    {
      printf("[Master] Read Humi(addr=0x%02X): %d.%d %%\r\n",
             SLAVE_REG_HUMI,
             g_master_sensor.humidity / 10, g_master_sensor.humidity % 10);
    }
    else
    {
      printf("[Master] Read Humi FAILED\r\n");
    }

    osDelay(500);

    /* 3. 单独读取光照寄存器 (SLAVE_REG_LIGHT, 2 字节) */
    if (Master_ReadLightByAddr() == 0)
    {
      /* 光照合法范围兜底校验 (从机生成范围 0 ~ 999) */
      if ((g_master_sensor.light >= 0) && (g_master_sensor.light <= 999))
      {
        printf("[Master] Read Light(addr=0x%02X): %d lux\r\n",
               SLAVE_REG_LIGHT, g_master_sensor.light);
      }
      else
      {
        printf("[Master] Light data INVALID, dropped\r\n");
      }
    }
    else
    {
      printf("[Master] Read Light FAILED\r\n");
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

  /* 按寄存器地址派发, 不依赖超时区分 cycle:
   *   主机写时间 (SLAVE_REG_TIME) -> Slave_ReadByAddr 接收并解析到 g_slave_time,
   *                                 随后刷新传感器供本轮读取。
   *   主机读传感器 (TEMP/HUMI/LIGHT) -> Slave_ReadByAddr 内部按需序列化并发送。
   * Slave_ReadByAddr 返回处理的寄存器地址, 超时/异常返回 0xFF。
   */
  for(;;)
  {
    uint8_t reg = Slave_ReadByAddr();

    if (reg == SLAVE_REG_TIME)
    {
      printf("[Slave]  Recv Time(addr=0x%02X): %04u-%02u-%02u %02u:%02u:%02u\r\n",
             SLAVE_REG_TIME,
             g_slave_time.year, g_slave_time.month, g_slave_time.day,
             g_slave_time.hour, g_slave_time.minute, g_slave_time.second);
      Slave_UpdateSensor();
    }
  }
  /* USER CODE END Slave_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

