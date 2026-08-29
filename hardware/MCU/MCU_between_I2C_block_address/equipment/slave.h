#ifndef SLAVE_H
#define SLAVE_H

#include "main.h"
#include "BSP_IIC.h"
#include "i2c.h"

/* 从机接收到的时间 */
extern TimeData_t g_slave_time;

/* 从机传感器数据 (模拟) */
extern SensorData_t g_slave_sensor;

/* 从机初始化 */
void Slave_Init(void);

/* 从机 I2C 错误恢复 (阻塞式无需恢复, 保留为空操作) */
void Slave_RecoverI2C(void);

/* 更新从机模拟传感器数据 (在从机任务中周期调用) */
void Slave_UpdateSensor(void);

/* 解析从主机接收的时间数据
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 */
void Slave_ParseTime(const uint8_t *buf, TimeData_t *out);

/* ===================== 带数据地址的从机接口 ===================== */

/* 从机寄存器文件 (主机通过寄存器地址访问) */
extern uint8_t g_slave_regs[SLAVE_REG_SIZE];

/* 从机带地址接收主机写入的数据
 * 主机使用 Mem_Write, 发送帧为 [Reg][Data...], 本函数一次接收全部。
 * 接收后将数据写入 g_slave_regs[reg ... reg+len-1], 并同步到 g_slave_time。
 * 返回: 0 = 成功, 1 = 失败/超时
 */
uint8_t Slave_ReceiveByAddr(void);

/* 从机带地址发送数据给主机
 * 主机使用 Mem_Read, 时序为 [Reg] (写阶段) + Sr [Data...] (读阶段)。
 * 本函数先阻塞接收 1 字节寄存器地址, 再阻塞发送该地址对应的数据。
 * 返回: 0 = 成功, 1 = 失败/超时
 */
uint8_t Slave_TransmitByAddr(void);

/* 将时间数据同步到从机寄存器文件 (SLAVE_REG_TIME 起始 7 字节) */
void Slave_SyncTimeToRegs(void);

/* 将传感器数据同步到从机寄存器文件
 * 温度 -> SLAVE_REG_TEMP, 湿度 -> SLAVE_REG_HUMI, 光照 -> SLAVE_REG_LIGHT (各 2 字节)
 */
void Slave_SyncSensorToRegs(void);

#endif
