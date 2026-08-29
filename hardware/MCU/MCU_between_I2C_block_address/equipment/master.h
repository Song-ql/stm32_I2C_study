#ifndef MASTER_H
#define MASTER_H

#include "main.h"
#include "BSP_IIC.h"

/* 主机数据缓冲区 */
typedef struct
{
    uint8_t tx_buf[I2C_FRAME_SIZE]; /* 发送给从机的时间数据 (7 字节) */
    uint8_t rx_buf[I2C_SENSOR_DATA_SIZE]; /* 接收从机的传感器数据 */
} MasterBuffer_t;

/* 主机数据缓冲区 */
extern MasterBuffer_t g_master_buffer;

/* 主机解析后的传感器数据 */
extern SensorData_t g_master_sensor;

/* 主机发送时间到从机
 * 参数: pTime - 时间数据指针
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_SendTime(const TimeData_t *pTime);

/* 主机读取从机传感器数据 (阻塞接收)
 * 返回: 0 = 成功 (数据已写入 g_master_buffer.rx_buf), 1 = 失败
 * 注意: 函数返回后可直接调用 Master_ParseSensor 解析, 无需轮询标志。
 */
uint8_t Master_ReadSensor(void);

/* 解析接收到的传感器数据 (小端序)
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 * 返回: 0 = 成功, out 已更新
 * 注意: 在 Master_ReadSensor 成功返回后直接调用即可。
 */
uint8_t Master_ParseSensor(const uint8_t *buf, SensorData_t *out);

#endif
