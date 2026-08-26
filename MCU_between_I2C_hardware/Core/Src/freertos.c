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
	
  /* Infinite loop */
  for(;;)
  {
    /* 1. 读取从机ID，校验通信是否正常 */
    Master_SetID(Master_ReadID());
    if (Master_GetID() != 0xAB)
    {
      /* ID校验失败，通信异常，等待后重试 */
      osDelay(100);
      continue;
    }

    /* 2. 读取状态寄存器，判断从机是否就绪 */
    Master_SetStatus(Master_ReadStatus());
    if ((Master_GetStatus() & 0x01) == 0x00)
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
    Master_SetTemp(Master_ReadTemperature());

    /* 5. 按寄存器地址连续读取湿度（高8位+低8位） */
    Master_SetHum(Master_ReadHumidity());

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
    /* 模拟传感器数据更新：将数据写入从机寄存器
       主机通过 I2C 按寄存器地址即可读到这些值 */

    /* 更新温度寄存器（高8位 + 低8位）- 使用 switch-case 寻址 */
    Reg_Write(REG_TEMP_H, (uint8_t)(sim_temp >> 8));
    Reg_Write(REG_TEMP_L, (uint8_t)(sim_temp & 0xFF));

    /* 更新湿度寄存器（高8位 + 低8位）- 使用 switch-case 寻址 */
    Reg_Write(REG_HUM_H, (uint8_t)(sim_hum >> 8));
    Reg_Write(REG_HUM_L, (uint8_t)(sim_hum & 0xFF));

    /* 模拟数据缓慢变化，便于观察主机读取效果 */
    sim_temp += 1;
    sim_hum  += 2;

    /* REG_STATUS 就绪标志翻转：受 REG_CTRL 控制
       REG_CTRL 定义：
         bit0     : 翻转使能 (1=允许翻转, 0=禁止翻转)
         bit7~bit1: 翻转周期 = (bit7~bit1) + 1 次循环
       示例：
         REG_CTRL = 0x00 → 禁止翻转
         REG_CTRL = 0x01 → 每 1 次循环翻转 1 次
         REG_CTRL = 0x13 → 每(0x09+1)=10 次循环翻转 1 次（兼容原默认行为） */
    cnt++;
    {
      uint8_t ctrl = Reg_Read(REG_CTRL);
      uint8_t enable  = ctrl & 0x01;
      uint8_t period  = (ctrl >> 1) + 1;  /* 翻转周期：1 ~ 128 */

      if (enable && (cnt >= period))
      {
        cnt = 0;
        /* 使用 switch-case 读写：先读后写，实现异或翻转 */
        Reg_Write(REG_STATUS, Reg_Read(REG_STATUS) ^ 0x01);
      }
      else if (!enable)
      {
        /* 翻转禁止时，计数器不累积 */
        cnt = 0;
      }
    }

    osDelay(100);
  }
  /* USER CODE END Slave_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

