#ifndef SLAVE_H
#define SLAVE_H

#include "main.h"
#include "BSP_IIC.h"
#include "i2c.h"

/* 从机数据缓冲区 */
typedef struct
{
    uint8_t rx_buf[I2C_FRAME_SIZE]; /* 接收主机发来的命令+数据 */
    uint8_t tx_buf[I2C_SENSOR_DATA_SIZE]; /* 传感器数据, 供主机读取 */
} SlaveBuffer_t;

/* 从机数据缓冲区 */
extern SlaveBuffer_t g_slave_buffer;

/* 从机接收到的时间 */
extern TimeData_t g_slave_time;

/* 从机传感器数据 (模拟) */
extern SensorData_t g_slave_sensor;

/* 从机初始化 */
void Slave_Init(void);

/* 更新从机模拟传感器数据 (在从机任务中周期调用) */
void Slave_UpdateSensor(void);

/* 解析从主机接收的时间帧 */
void Slave_ParseTime(const uint8_t *buf);

/* 打包从机传感器数据到 tx_buf (小端序) */
void Slave_PackSensor(uint8_t *buf);

#endif
