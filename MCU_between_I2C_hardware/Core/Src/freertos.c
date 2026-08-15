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
#include "slave.h"
#include "master.h"
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
  uint16_t temperature; /* 模拟温度原始值 */
  uint16_t humidity;    /* 模拟湿度原始值 */
  uint8_t  status;      /* 模拟状态寄存器值 */
  /* Infinite loop */
  for(;;)
  {
    /* 1. 读取从机ID，校验通信是否正常 */
    id = Master_ReadID();
    if (id != 0xAB)
    {
      /* ID校验失败，通信异常，等待后重试 */
      osDelay(100);
      continue;
    }

    /* 2. 读取状态寄存器，判断从机是否就绪 */
    status = Master_ReadStatus();
    if ((status & 0x01) == 0x00)
    {
      /* 从机未就绪，等待后重试 */
      osDelay(10);
      continue;
    }

    /* 3. 写控制寄存器，启动一次测量 */
    Master_WriteCtrl(0x01);

    /* 等待从机完成测量 */
    osDelay(5);

    /* 4. 按寄存器地址连续读取温度（高8位+低8位） */
    temperature = Master_ReadTemperature();

    /* 5. 按寄存器地址连续读取湿度（高8位+低8位） */
    humidity = Master_ReadHumidity();

    /* 6. 此处可将 temperature / humidity 发送到串口、OLED 等
       例如: printf("Temp=%d, Hum=%d\r\n", temperature, humidity); */

    osDelay(100);
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
  uint16_t sim_temp = 0x1234; /* 模拟温度原始值 */
  uint16_t sim_hum  = 0x5678; /* 模拟湿度原始值 */
  uint8_t  cnt = 0;
  /* Infinite loop */
  for(;;)
  {
    /* 模拟传感器数据更新：将数据写入从机寄存器数组
       主机通过 I2C 按寄存器地址即可读到这些值 */

    /* 更新温度寄存器（高8位 + 低8位） */
    slave_reg[REG_TEMP_H] = (uint8_t)(sim_temp >> 8);
    slave_reg[REG_TEMP_L] = (uint8_t)(sim_temp & 0xFF);

    /* 更新湿度寄存器（高8位 + 低8位） */
    slave_reg[REG_HUM_H] = (uint8_t)(sim_hum >> 8);
    slave_reg[REG_HUM_L] = (uint8_t)(sim_hum & 0xFF);

    /* 模拟数据缓慢变化，便于观察主机读取效果 */
    sim_temp += 1;
    sim_hum  += 2;

    /* 每10次更新翻转状态寄存器的就绪标志，测试主机状态判断逻辑 */
    cnt++;
    if (cnt >= 10)
    {
      cnt = 0;
      slave_reg[REG_STATUS] ^= 0x01;
    }

    osDelay(100);
  }
  /* USER CODE END Slave_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

