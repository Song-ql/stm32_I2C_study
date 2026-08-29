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

/* 主机 DMA 接收完成标志 */
extern volatile uint8_t g_master_rx_done;

/* 主机 DMA 接收错误标志 (中断置位, 任务清除) */
extern volatile uint8_t g_master_rx_error;

/* 主机 DMA 发送完成标志 */
extern volatile uint8_t g_master_tx_done;

/* 主机 DMA 发送错误标志 (中断置位, 任务清除) */
extern volatile uint8_t g_master_tx_error;

/* 主机当前传输方向: 'T'=发送, 'R'=接收, 0=空闲 */
extern volatile uint8_t g_master_current_xfer;

/* 主机解析后的传感器数据 */
extern SensorData_t g_master_sensor;

/* 主机发送时间到从机 (DMA发送)
 * 返回: 0 = 成功启动, 1 = 失败
 * 注意: 需轮询 g_master_tx_done, 完成后确认发送结果。
 */
uint8_t Master_SendTime(const TimeData_t *pTime);

/* 主机读取从机传感器数据 (DMA接收)
 * 返回: 0 = 成功启动, 1 = 失败
 * 注意: 需轮询 g_master_rx_done, 完成后调用 Master_ParseSensor 解析。
 */
uint8_t Master_ReadSensor(void);

/* 解析接收到的传感器数据 (小端序)
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 * 应在 g_master_rx_done 置位后调用。
 */
void Master_ParseSensor(const uint8_t *buf, SensorData_t *out);

/* 主机 I2C 错误恢复 (在任务上下文中调用)
 * 检查 I2C1 状态, 若异常则重新初始化以恢复总线。
 */
void Master_RecoverI2C(void);

#endif
