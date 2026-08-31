#ifndef SLAVE_H
#define SLAVE_H

#include "main.h"
#include "BSP_IIC.h"
#include "i2c.h"
#include "BSP_UART.h"

/* 从机数据缓冲区 */
typedef struct
{
    uint8_t rx_buf[I2C_FRAME_SIZE]; /* 接收主机的时间数据 */
    uint8_t tx_buf[I2C_SENSOR_DATA_SIZE]; /* 传感器数据, 供主机读取 */
} SlaveBuffer_t;

/* 从机数据缓冲区 */
extern SlaveBuffer_t g_slave_buffer;

/* 从机接收到的时间 */
extern TimeData_t g_slave_time;

/* 从机传感器数据 (模拟) */
extern SensorData_t g_slave_sensor;

/* 从机 I2C 监听模式需要恢复的标志 (由中断置位, 任务清除) */
extern volatile uint8_t g_slave_i2c_need_recover;

/* 从机 DMA 接收完成标志 (中断置位, 任务清除) */
extern volatile uint8_t g_slave_rx_done;

/* 从机初始化 */
void Slave_Init(void);

/* 从机 I2C 监听模式错误恢复 (在任务上下文中调用) */
void Slave_RecoverI2C(void);

/* 更新从机模拟传感器数据 (在从机任务中周期调用) */
void Slave_UpdateSensor(void);

/* 解析从主机接收的时间数据
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 */
void Slave_ParseTime(const uint8_t *buf, TimeData_t *out);

/* 打包从机传感器数据到 buf (小端序)
 * 参数: sensor - 传感器数据; buf - 输出字节缓冲区
 */
void Slave_PackSensor(const SensorData_t *sensor, uint8_t *buf);

#endif
