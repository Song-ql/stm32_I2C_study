#ifndef SLAVE_H
#define SLAVE_H

#include "main.h"
#include "BSP_IIC.h"
#include "i2c.h"

/* 从机接收到的时间 */
extern TimeData_t g_slave_time;

/* 从机传感器数据 (模拟) */
extern SensorData_t g_slave_sensor;

/* 更新从机模拟传感器数据 (在从机任务中周期调用) */
void Slave_UpdateSensor(void);

/* 从机带地址数据处理: 返回处理的寄存器地址, 超时/异常返回 0xFF */
uint8_t Slave_ReadByAddr(void);

#endif
