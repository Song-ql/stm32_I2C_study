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

/* 主机读: 传感器帧长度 = 8 字节 (6 字节数据 + 1 字节校验和 + 1 字节填充)
 * 说明: F1 主机中断接收时 HAL 对最后两字节背靠背连读 DR, 实测末字节
 * 会被重复成倒数第二字节 (传输竞争), 因此帧尾放校验和 + 填充字节,
 * 使重复损坏只落在填充位; 主机以校验和验证帧有效性。
 */
#define I2C_SENSOR_DATA_SIZE      8

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
 * 帧传输时 6 字节数据后附加 1 字节校验和 + 1 字节填充, 共 8 字节
 */
typedef struct
{
    int16_t  temperature;   /* 温度 x10, 单位 0.1°C */
    int16_t  humidity;      /* 湿度 x10, 单位 0.1% */
    int16_t  light;         /* 光照强度, 单位 lux */
} SensorData_t;   /* 共 6 字节 */

#endif
