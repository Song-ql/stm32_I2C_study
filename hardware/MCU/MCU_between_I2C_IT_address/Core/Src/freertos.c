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

/* 串口输出互斥锁, 防止 Master/Slave 任务 printf 输出交织 */
osMutexId g_uart_mutex;

/* 测试结果统计 */
static uint32_t g_test_pass = 0;
static uint32_t g_test_fail = 0;

/* USER CODE END Variables */
osThreadId MasterTaskHandle;
osThreadId SlaveTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* 带超时等待的寄存器读取 (内部轮询 g_master_rx_done)
 * 参数: reg_addr - 寄存器地址; len - 读取长度; timeout_ms - 超时
 * 返回: 0 = 成功(数据在 g_master_buffer.rx_buf), 1 = 超时或启动失败
 */
static uint8_t Test_ReadRegWait(uint8_t reg_addr, uint16_t len, uint32_t timeout_ms);

/* 打印测试结果并更新统计
 * 参数: name - 测试项名称; pass - 1=通过, 0=失败; fmt... - 附加信息
 */
static void Test_Report(const char *name, uint8_t pass, const char *fmt, ...);

/* 线程安全的 printf (加串口互斥锁) */
static void Test_Printf(const char *fmt, ...);

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
  /* 串口输出互斥锁, 保护 printf 不被多任务打断 */
  osMutexDef(uartMutex);
  g_uart_mutex = osMutexCreate(osMutex(uartMutex));
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
  *         基于数据地址(ID)协议的完整读写测试套件
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Master_Task */
void Master_Task(void const * argument)
{
  /* USER CODE BEGIN Master_Task */
	TimeData_t w_time;
	TimeData_t r_time;
	uint8_t version;
	uint8_t led_w;
	uint8_t led_r;
	uint8_t i;
	uint8_t pass;

  /* 测试套件循环运行 */
  for(;;)
  {
    Test_Printf("\r\n========== I2C Register Read/Write Test Suite ==========\r\n");
    Test_Printf("Reg Map: TIME(0x00,RW,7B) SENSOR(0x01,RO,6B) VERSION(0x02,RO,1B) LED(0x03,RW,1B)\r\n");

    /* --------------------------------------------------------------- */
    /* 测试 1: 读固件版本号 (REG_ADDR_VERSION, 只读, 1 字节)          */
    /* 预期: 返回 FIRMWARE_VERSION                                     */
    /* --------------------------------------------------------------- */
    if (Test_ReadRegWait(REG_ADDR_VERSION, 1, 1000) == 0)
    {
      version = g_master_buffer.rx_buf[0];
      pass = (version == FIRMWARE_VERSION) ? 1 : 0;
      Test_Report("Read REG_VERSION", pass, "got=0x%02X expected=0x%02X", version, FIRMWARE_VERSION);
    }
    else
    {
      Test_Report("Read REG_VERSION", 0, "timeout or failed");
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 测试 2: LED 写 0 并读回 (REG_ADDR_LED, 读写, 1 字节)           */
    /* 预期: 读回值 == 写入值 0                                        */
    /* --------------------------------------------------------------- */
    led_w = 0;
    if (Master_WriteReg(REG_ADDR_LED, &led_w, 1) == 0)
    {
      osDelay(50);  /* 等待从机处理写入 */
      if (Test_ReadRegWait(REG_ADDR_LED, 1, 1000) == 0)
      {
        led_r = g_master_buffer.rx_buf[0];
        pass = (led_r == led_w) ? 1 : 0;
        Test_Report("Write/Read REG_LED=0", pass, "wrote=%u readback=%u", led_w, led_r);
      }
      else
      {
        Test_Report("Write/Read REG_LED=0", 0, "readback timeout");
      }
    }
    else
    {
      Test_Report("Write/Read REG_LED=0", 0, "write failed");
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 测试 3: LED 写 1 并读回 (REG_ADDR_LED, 读写, 1 字节)           */
    /* 预期: 读回值 == 写入值 1                                        */
    /* --------------------------------------------------------------- */
    led_w = 1;
    if (Master_WriteReg(REG_ADDR_LED, &led_w, 1) == 0)
    {
      osDelay(50);
      if (Test_ReadRegWait(REG_ADDR_LED, 1, 1000) == 0)
      {
        led_r = g_master_buffer.rx_buf[0];
        pass = (led_r == led_w) ? 1 : 0;
        Test_Report("Write/Read REG_LED=1", pass, "wrote=%u readback=%u", led_w, led_r);
      }
      else
      {
        Test_Report("Write/Read REG_LED=1", 0, "readback timeout");
      }
    }
    else
    {
      Test_Report("Write/Read REG_LED=1", 0, "write failed");
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 测试 4: 时间读写回环 (REG_ADDR_TIME, 读写, 7 字节)             */
    /* 预期: 读回的时间数据与写入的完全一致 (逐字节比较)               */
    /* --------------------------------------------------------------- */
    w_time.year   = 2026;
    w_time.month  = 8;
    w_time.day    = 31;
    w_time.hour   = 14;
    w_time.minute = 30;
    w_time.second = 45;

    if (Master_SendTime(&w_time) == 0)
    {
      osDelay(50);
      if (Test_ReadRegWait(REG_ADDR_TIME, sizeof(TimeData_t), 1000) == 0)
      {
        /* 解析读回的时间数据 (小端序, 与 Slave_PrepareTx 打包格式一致) */
        r_time.year   = (uint16_t)g_master_buffer.rx_buf[0] | ((uint16_t)g_master_buffer.rx_buf[1] << 8);
        r_time.month  = g_master_buffer.rx_buf[2];
        r_time.day    = g_master_buffer.rx_buf[3];
        r_time.hour   = g_master_buffer.rx_buf[4];
        r_time.minute = g_master_buffer.rx_buf[5];
        r_time.second = g_master_buffer.rx_buf[6];

        pass = (memcmp(&w_time, &r_time, sizeof(TimeData_t)) == 0) ? 1 : 0;
        Test_Report("Write/Read REG_TIME", pass,
                    "wrote %04u-%02u-%02u %02u:%02u:%02u",
                    r_time.year, r_time.month, r_time.day,
                    r_time.hour, r_time.minute, r_time.second);
      }
      else
      {
        Test_Report("Write/Read REG_TIME", 0, "readback timeout");
      }
    }
    else
    {
      Test_Report("Write/Read REG_TIME", 0, "write failed");
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 测试 5: 读传感器数据并校验范围 (REG_ADDR_SENSOR, 只读, 6 字节) */
    /* 预期: 温度 200~300, 湿度 400~600, 光照 0~999                    */
    /* --------------------------------------------------------------- */
    if (Test_ReadRegWait(REG_ADDR_SENSOR, sizeof(SensorData_t), 1000) == 0)
    {
      int16_t temp = (int16_t)(g_master_buffer.rx_buf[0] | ((uint16_t)g_master_buffer.rx_buf[1] << 8));
      int16_t humi = (int16_t)(g_master_buffer.rx_buf[2] | ((uint16_t)g_master_buffer.rx_buf[3] << 8));
      int16_t light = (int16_t)(g_master_buffer.rx_buf[4] | ((uint16_t)g_master_buffer.rx_buf[5] << 8));

      pass = ((temp >= 200) && (temp <= 300) &&
              (humi >= 400) && (humi <= 600) &&
              (light >= 0) && (light <= 999)) ? 1 : 0;
      Test_Report("Read REG_SENSOR range", pass,
                  "Temp=%d.%d Humi=%d.%d Light=%d",
                  temp / 10, temp % 10, humi / 10, humi % 10, light);

      /* 打印原始字节便于调试 */
      Test_Printf("        raw bytes:");
      for (i = 0; i < 6; i++)
      {
        Test_Printf(" %02X", g_master_buffer.rx_buf[i]);
      }
      Test_Printf("\r\n");
    }
    else
    {
      Test_Report("Read REG_SENSOR range", 0, "timeout or failed");
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 测试 6: 只读寄存器写保护 (REG_ADDR_VERSION)                    */
    /* 步骤: 先读版本 -> 尝试写入 0x99 -> 再读版本 -> 应保持不变       */
    /* 预期: 写入后读回值仍为 FIRMWARE_VERSION (从机忽略对只读寄存器写入) */
    /* --------------------------------------------------------------- */
    {
      uint8_t ver_before, ver_after;
      uint8_t bad_val = 0x99;

      /* 先读当前版本 */
      if (Test_ReadRegWait(REG_ADDR_VERSION, 1, 1000) != 0)
      {
        Test_Report("RO write-protect VERSION", 0, "read before failed");
      }
      else
      {
        ver_before = g_master_buffer.rx_buf[0];
        /* 尝试写入只读寄存器 (应被从机忽略) */
        Master_WriteReg(REG_ADDR_VERSION, &bad_val, 1);
        osDelay(50);
        /* 再读, 应保持不变 */
        if (Test_ReadRegWait(REG_ADDR_VERSION, 1, 1000) != 0)
        {
          Test_Report("RO write-protect VERSION", 0, "read after failed");
        }
        else
        {
          ver_after = g_master_buffer.rx_buf[0];
          pass = (ver_after == ver_before) ? 1 : 0;
          Test_Report("RO write-protect VERSION", pass,
                      "before=0x%02X wrote=0x%02X after=0x%02X",
                      ver_before, bad_val, ver_after);
        }
      }
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 测试 7: 无效寄存器地址读取 (地址 >= REG_ADDR_MAX)              */
    /* 预期: 从机返回 0 字节数据 (g_slave_tx_len=0), 主机读取超时或得到0 */
    /* --------------------------------------------------------------- */
    {
      uint8_t invalid_addr = REG_ADDR_MAX;  /* 越界地址 */
      if (Test_ReadRegWait(invalid_addr, 1, 500) != 0)
      {
        /* 超时是预期行为 (从机无数据可发) */
        Test_Report("Read invalid addr(0x%02X)", 1, "timeout as expected", invalid_addr);
      }
      else
      {
        /* 即使返回了数据, 也应为 0 (从机对无效地址返回长度 0) */
        uint8_t val = g_master_buffer.rx_buf[0];
        pass = (val == 0) ? 1 : 0;
        Test_Report("Read invalid addr(0x%02X)", pass, "got=0x%02X (expect 0 or timeout)", invalid_addr, val);
      }
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 测试 8: 短数据写入 (写 1 字节到 TIME 寄存器, 长度不足)          */
    /* 预期: 从机因长度不足忽略写入, 读回 TIME 保持上一次的值          */
    /* --------------------------------------------------------------- */
    {
      uint8_t short_data = 0xAB;
      TimeData_t time_before_short;

      /* 保存当前时间 */
      if (Test_ReadRegWait(REG_ADDR_TIME, sizeof(TimeData_t), 1000) == 0)
      {
        time_before_short.year   = (uint16_t)g_master_buffer.rx_buf[0] | ((uint16_t)g_master_buffer.rx_buf[1] << 8);
        time_before_short.month  = g_master_buffer.rx_buf[2];
        time_before_short.day    = g_master_buffer.rx_buf[3];
        time_before_short.hour   = g_master_buffer.rx_buf[4];
        time_before_short.minute = g_master_buffer.rx_buf[5];
        time_before_short.second = g_master_buffer.rx_buf[6];
      }
      else
      {
        memset(&time_before_short, 0, sizeof(time_before_short));
      }

      /* 写 1 字节到 TIME (长度不足 7, 应被忽略) */
      Master_WriteReg(REG_ADDR_TIME, &short_data, 1);
      osDelay(50);

      /* 读回, 应与写入前一致 */
      if (Test_ReadRegWait(REG_ADDR_TIME, sizeof(TimeData_t), 1000) == 0)
      {
        r_time.year   = (uint16_t)g_master_buffer.rx_buf[0] | ((uint16_t)g_master_buffer.rx_buf[1] << 8);
        r_time.month  = g_master_buffer.rx_buf[2];
        r_time.day    = g_master_buffer.rx_buf[3];
        r_time.hour   = g_master_buffer.rx_buf[4];
        r_time.minute = g_master_buffer.rx_buf[5];
        r_time.second = g_master_buffer.rx_buf[6];

        pass = (memcmp(&time_before_short, &r_time, sizeof(TimeData_t)) == 0) ? 1 : 0;
        Test_Report("Short write to TIME ignored", pass,
                    "time unchanged after 1-byte write");
      }
      else
      {
        Test_Report("Short write to TIME ignored", 0, "readback timeout");
      }
    }
    osDelay(200);

    /* --------------------------------------------------------------- */
    /* 打印本轮测试汇总                                               */
    /* --------------------------------------------------------------- */
    Test_Printf("----------------------------------------------------------\r\n");
    Test_Printf("Test Summary: PASS=%u  FAIL=%u\r\n",
                (unsigned)g_test_pass, (unsigned)g_test_fail);
    Test_Printf("==========================================================\r\n");

    osDelay(2000);  /* 每轮测试间隔 2 秒 */
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

  /* Infinite loop: 更新模拟传感器数据, 处理主机寄存器写入, 打印当前状态 */
  for(;;)
  {
    /* 检查并恢复 I2C 监听模式 (处理非 AF 错误) */
    Slave_RecoverI2C();

    /* 更新从机模拟传感器数据 */
    Slave_UpdateSensor();

    /* 接收到主机写事务后, 在任务中处理 (中断只置标志)
     * rx_buf[0] = 寄存器地址, rx_buf[1..] = 数据, rx_data_len = 数据长度
     */
    if (g_slave_rx_done != 0)
    {
      g_slave_rx_done = 0;
      Slave_ProcessWrite(g_slave_buffer.rx_buf[0],
                         &g_slave_buffer.rx_buf[1],
                         (uint8_t)g_slave_rx_data_len);
    }

    /* 打印从机当前状态: 收到的主机时间、传感器数据、LED 状态 */
    Test_Printf("[Slave] Time: %04u-%02u-%02u %02u:%02u:%02u | "
                "Temp=%d.%d Humi=%d.%d Light=%d | LED=%u\r\n",
                g_slave_time.year, g_slave_time.month, g_slave_time.day,
                g_slave_time.hour, g_slave_time.minute, g_slave_time.second,
                g_slave_sensor.temperature / 10, g_slave_sensor.temperature % 10,
                g_slave_sensor.humidity / 10, g_slave_sensor.humidity % 10,
                g_slave_sensor.light,
                (unsigned)g_slave_led);

    osDelay(1000);
  }
  /* USER CODE END Slave_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

#include <stdarg.h>

/* 线程安全的 printf: 加串口互斥锁, 防止 Master/Slave 输出交织 */
static void Test_Printf(const char *fmt, ...)
{
    va_list args;
    osMutexWait(g_uart_mutex, osWaitForever);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    osMutexRelease(g_uart_mutex);
}

/* 带超时等待的寄存器读取 */
static uint8_t Test_ReadRegWait(uint8_t reg_addr, uint16_t len, uint32_t timeout_ms)
{
    uint32_t wait_start;

    if (Master_ReadReg(reg_addr, len) != 0)
    {
        return 1;
    }

    wait_start = HAL_GetTick();
    while ((g_master_rx_done == 0) && ((HAL_GetTick() - wait_start) < timeout_ms))
    {
        osDelay(1);
    }

    return (g_master_rx_done != 0) ? 0 : 1;
}

/* 打印测试结果并更新 PASS/FAIL 统计 */
static void Test_Report(const char *name, uint8_t pass, const char *fmt, ...)
{
    va_list args;
    osMutexWait(g_uart_mutex, osWaitForever);

    if (pass)
    {
        g_test_pass++;
        printf("[TEST] PASS: %s", name);
    }
    else
    {
        g_test_fail++;
        printf("[TEST] FAIL: %s", name);
    }

    if (fmt != NULL)
    {
        printf(" | ");
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
    printf("\r\n");

    osMutexRelease(g_uart_mutex);
}

/* USER CODE END Application */

