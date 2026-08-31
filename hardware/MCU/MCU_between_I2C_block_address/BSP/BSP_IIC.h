#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "stm32f1xx_hal.h"

/* 从机地址 (8 位) */
#define BSP_IIC_SLAVE_ADDR_8BIT (0x50<<1)

/* I2C 传输超时时间(ms) */
#define BSP_IIC_TIMEOUT 1000

/* ===================== 通信协议定义 ===================== */

/* 主机写: 时间数据长度 = 7 字节 (TimeData_t) */
#define I2C_FRAME_TIME_SIZE 7

/* 单个传感器数据项长度 (温度/湿度/光照 各 2 字节, 小端) */
#define I2C_SENSOR_SIZE 2

/* 从机寄存器地址定义
 * 温度/湿度/光照 各占独立寄存器地址, 每次单独读取 2 字节。
 */
#define SLAVE_REG_TIME 0x00   /* 时间数据起始地址 (7 字节) */
#define SLAVE_REG_TEMP 0x10   /* 温度寄存器 (2 字节, 小端) */
#define SLAVE_REG_HUMI 0x12   /* 湿度寄存器 (2 字节, 小端) */
#define SLAVE_REG_LIGHT 0x14   /* 光照强度寄存器 (2 字节, 小端) */

/* 时间数据结构 (主机发送给从机) */
typedef struct
{
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} TimeData_t;   /* 共 7 字节 */

/* 传感器数据结构 (主机读取从机) */
typedef struct
{
    int16_t  temperature;   /* 温度 x10, 单位 0.1°C */
    int16_t  humidity;      /* 湿度 x10, 单位 0.1% */
    int16_t  light;         /* 光照强度, 单位 lux */
} SensorData_t;   /* 共 6 字节 */

/* ===================== I2C 外设自恢复 ===================== */

/* 连续失败/超时阈值, 达到后触发 I2C 外设恢复 (清除 STM32F1 BUSY 标志锁死) */
#define BSP_IIC_RECOVER_THRESHOLD 3

/* I2C 互斥量等待超时(ms): 另一侧恢复约几十 ms, 200ms 余量足够 */
#define BSP_IIC_MUTEX_TIMEOUT 200

/* I2C 外设恢复: DeInit + 最多 9 个 SCL 脉冲清总线 + STOP + ReInit
 * 参数:
 *   hi2c      - I2C 句柄 (&hi2c1 / &hi2c2)
 *   port      - SCL/SDA 所在 GPIO 端口 (GPIOB)
 *   scl_pin   - SCL 引脚号
 *   sda_pin   - SDA 引脚号
 *   reinit    - 重新初始化函数 (MX_I2C1_Init / MX_I2C2_Init), 可为 NULL
 */
void BSP_IIC_Recover(I2C_HandleTypeDef *hi2c,
                     GPIO_TypeDef *port, uint16_t scl_pin, uint16_t sda_pin,
                     void (*reinit)(void));

#endif
