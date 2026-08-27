#ifndef MASTER_H
#define MASTER_H

#include "main.h"
#include "BSP_IIC.h"

/* 主机数据缓冲区 */
typedef struct
{
    uint8_t tx_buf[I2C_FRAME_SIZE]; /* 主机发送从机数据帧总长度 = 1 字节命令 + 7 字节 */
    uint8_t rx_buf[I2C_SENSOR_DATA_SIZE]; /* 主机接收从机传感器数据 */
} MasterBuffer_t;

/* 主机数据缓冲区 */
extern MasterBuffer_t g_master_buffer;

/* 主机中断接收完成标志 */
extern volatile uint8_t g_master_rx_done;

/* 主机解析后的传感器数据 */
extern SensorData_t g_master_sensor;

/* 主机发送时间到从机
 * 参数: pTime - 时间数据指针
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_SendTime(const TimeData_t *pTime);

/* 主机启动读取从机传感器数据 (发送命令 + 启动中断接收)
 * 返回: 0 = 命令发送成功且中断接收已启动, 1 = 失败
 * 注意: 传感器数据不会立即返回, 需轮询 g_master_rx_done, 完成后从 g_master_sensor 读取。
 */
uint8_t Master_ReadSensor(void);

#endif
