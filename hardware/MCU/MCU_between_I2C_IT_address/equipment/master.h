#ifndef MASTER_H
#define MASTER_H

#include "main.h"
#include "BSP_IIC.h"

/* 主机数据缓冲区
 * tx_buf: 写事务, 格式 [寄存器地址(1)][数据(I2C_MAX_DATA_SIZE)]
 * rx_buf: 读事务接收数据, 最大 I2C_MAX_DATA_SIZE 字节
 */
typedef struct
{
    uint8_t tx_buf[I2C_FRAME_SIZE];
    uint8_t rx_buf[I2C_MAX_DATA_SIZE];
} MasterBuffer_t;

/* 主机数据缓冲区 */
extern MasterBuffer_t g_master_buffer;

/* 主机中断接收完成标志 */
extern volatile uint8_t g_master_rx_done;

/* 主机解析后的传感器数据 */
extern SensorData_t g_master_sensor;

/* 主机向指定寄存器地址写入数据 (阻塞方式)
 * 参数: reg_addr - 寄存器地址; data - 数据指针; len - 数据长度
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_WriteReg(uint8_t reg_addr, const uint8_t *data, uint16_t len);

/* 主机从指定寄存器地址读取数据 (中断接收方式)
 * 参数: reg_addr - 寄存器地址; len - 期望读取的数据长度
 * 返回: 0 = 成功启动, 1 = 失败
 * 注意: 需轮询 g_master_rx_done, 完成后数据在 g_master_buffer.rx_buf 中。
 */
uint8_t Master_ReadReg(uint8_t reg_addr, uint16_t len);

/* 主机发送时间到从机 (写 REG_ADDR_TIME)
 * 参数: pTime - 时间数据指针
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_SendTime(const TimeData_t *pTime);

/* 主机读取从机传感器数据 (读 REG_ADDR_SENSOR, 中断接收)
 * 返回: 0 = 成功启动, 1 = 失败
 * 注意: 需轮询 g_master_rx_done, 完成后调用 Master_ParseSensor 解析。
 */
uint8_t Master_ReadSensor(void);

/* 解析接收到的传感器数据 (小端序)
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 * 返回: 0 = 成功, out 已更新
 * 应在 g_master_rx_done 置位后调用。
 */
uint8_t Master_ParseSensor(const uint8_t *buf, SensorData_t *out);

#endif
