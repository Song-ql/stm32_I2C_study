#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "stm32f1xx_hal.h"

/* 从机地址 (8 位) */
#define BSP_IIC_SLAVE_ADDR_8BIT   (0x50<<1)

/* I2C 传输超时时间(ms) */
#define BSP_IIC_TIMEOUT           1000

/* ===================== 通信协议定义 ===================== */

/* 主机写: 时间数据长度 = 7 字节 (TimeData_t) */
#define I2C_FRAME_SIZE            7

/* 主机读: 传感器数据长度 = 6 字节 (SensorData_t) */
#define I2C_SENSOR_DATA_SIZE      6

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

/* 传感器数据结构 (主机读取从机)
 * 温度/湿度 放大 10 倍存储, 例如 25.5°C -> 255, 60.5% -> 605
 * 光照强度 单位 lux
 */
typedef struct
{
    int16_t  temperature;   /* 温度 x10, 单位 0.1°C */
    int16_t  humidity;      /* 湿度 x10, 单位 0.1% */
    int16_t  light;         /* 光照强度, 单位 lux */
} SensorData_t;   /* 共 6 字节 */

#endif
