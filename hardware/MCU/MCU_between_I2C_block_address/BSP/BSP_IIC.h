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

#endif
