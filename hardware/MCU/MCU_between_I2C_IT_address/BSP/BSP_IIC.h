#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "stm32f1xx_hal.h"

/* 从机地址 (8 位) */
#define BSP_IIC_SLAVE_ADDR_8BIT   (0x50<<1)

/* I2C 传输超时时间(ms) */
#define BSP_IIC_TIMEOUT           1000

/* ===================== 通信协议定义 (基于数据地址/ID 的寄存器式协议) ===================== */

/* 主机写事务格式: [从机地址+W][寄存器地址(ID)][数据...]
 * 主机读事务格式: [从机地址+W][寄存器地址(ID)][Sr][从机地址+R][数据...]
 * 通过寄存器地址(ID)区分读写的具体数据。
 */

/* 单个寄存器的最大数据长度 (时间数据 7 字节为最大) */
#define I2C_MAX_DATA_SIZE         7

/* 主机写帧最大长度 = 1 字节地址 + I2C_MAX_DATA_SIZE 字节数据 */
#define I2C_FRAME_SIZE            (1 + I2C_MAX_DATA_SIZE)

/* 主机读: 传感器帧长度 = 6 字节 (温度 + 湿度 + 光照, 各 2 字节小端) */
#define I2C_SENSOR_DATA_SIZE      6

/* ===================== 数据地址 ID (寄存器地址) 定义 ===================== */

#define REG_ADDR_TIME             0x00   /* 时间数据, 7 字节, 读写 */
#define REG_ADDR_SENSOR           0x01   /* 传感器数据, 6 字节, 只读 */
#define REG_ADDR_VERSION          0x02   /* 固件版本号, 1 字节, 只读 */
#define REG_ADDR_LED              0x03   /* LED 控制, 1 字节, 读写 */
#define REG_ADDR_MAX              0x04

/* 寄存器访问权限 */
#define REG_ACCESS_RO             0x00   /* 只读 */
#define REG_ACCESS_RW             0x01   /* 读写 */

/* 寄存器信息表项 */
typedef struct
{
    uint8_t size;       /* 数据长度 (字节) */
    uint8_t access;     /* 访问权限: REG_ACCESS_RO / REG_ACCESS_RW */
} RegInfo_t;

/* 从机端寄存器信息表 (在 slave.c 中定义) */
extern const RegInfo_t g_reg_table[REG_ADDR_MAX];

/* 固件版本号 */
#define FIRMWARE_VERSION          0x01

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
