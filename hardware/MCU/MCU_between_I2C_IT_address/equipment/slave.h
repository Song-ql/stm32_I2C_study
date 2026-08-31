#ifndef SLAVE_H
#define SLAVE_H

#include "main.h"
#include "BSP_IIC.h"
#include "i2c.h"

/* 从机数据缓冲区
 * rx_buf: 接收主机写事务, 格式 [寄存器地址(1)][数据(I2C_MAX_DATA_SIZE)]
 * tx_buf: 供主机读事务读取, 最大 I2C_MAX_DATA_SIZE 字节
 */
typedef struct
{
    uint8_t rx_buf[I2C_FRAME_SIZE];
    uint8_t tx_buf[I2C_MAX_DATA_SIZE];
} SlaveBuffer_t;

/* 从机数据缓冲区 */
extern SlaveBuffer_t g_slave_buffer;

/* 从机接收到的时间 */
extern TimeData_t g_slave_time;

/* 从机传感器数据 (模拟) */
extern SensorData_t g_slave_sensor;

/* 从机固件版本号 */
extern uint8_t g_slave_version;

/* 从机 LED 控制值 */
extern volatile uint8_t g_slave_led;

/* 从机当前寄存器地址指针 (主机写的第一个字节, 用于读事务定位数据) */
extern volatile uint8_t g_slave_reg_addr;

/* 从机当前发送数据长度 (根据寄存器地址确定) */
extern volatile uint8_t g_slave_tx_len;

/* 从机接收阶段: 0=等待寄存器地址, 1=等待数据 */
extern volatile uint8_t g_slave_rx_phase;

/* 从机 I2C 监听模式需要恢复的标志 (由中断置位, 任务清除) */
extern volatile uint8_t g_slave_i2c_need_recover;

/* 从机接收完成标志 (中断置位, 任务清除) */
extern volatile uint8_t g_slave_rx_done;

/* 从机实际接收到的数据长度 (不含寄存器地址字节) */
extern volatile uint8_t g_slave_rx_data_len;

/* 从机初始化 */
void Slave_Init(void);

/* 从机 I2C 监听模式错误恢复 (在任务上下文中调用) */
void Slave_RecoverI2C(void);

/* 更新从机模拟传感器数据 (在从机任务中周期调用) */
void Slave_UpdateSensor(void);

/* 根据寄存器地址准备发送数据到 tx_buf, 并设置 g_slave_tx_len
 * 参数: reg_addr - 寄存器地址
 */
void Slave_PrepareTx(uint8_t reg_addr);

/* 处理主机写入的寄存器数据
 * 参数: reg_addr - 寄存器地址; data - 数据指针; len - 数据长度
 */
void Slave_ProcessWrite(uint8_t reg_addr, const uint8_t *data, uint8_t len);

/* 解析从主机接收的时间数据
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 */
void Slave_ParseTime(const uint8_t *buf, TimeData_t *out);

/* 打包从机传感器数据到 buf (小端序)
 * 参数: sensor - 传感器数据; buf - 输出字节缓冲区
 */
void Slave_PackSensor(const SensorData_t *sensor, uint8_t *buf);

#endif
