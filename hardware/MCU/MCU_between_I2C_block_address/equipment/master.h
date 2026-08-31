#ifndef MASTER_H
#define MASTER_H

#include "main.h"
#include "BSP_IIC.h"

/* 主机数据缓冲区 */
typedef struct
{
    uint8_t tx_buf[I2C_FRAME_TIME_SIZE]; /* 发送给从机的时间数据 (7 字节) */
} MasterBuffer_t;

/* 主机数据缓冲区 */
extern MasterBuffer_t g_master_buffer;

/* 主机解析后的传感器数据 */
extern SensorData_t g_master_sensor;

/* ===================== 带数据地址的主机接口 ===================== */

/* 主机带地址发送时间到从机 (写入 SLAVE_REG_TIME, 7 字节)
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_SendTimeByAddr(const TimeData_t *pTime);

/* 主机单独读取温度寄存器 (SLAVE_REG_TEMP, 2 字节)
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_ReadTempByAddr(void);

/* 主机单独读取湿度寄存器 (SLAVE_REG_HUMI, 2 字节)
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_ReadHumiByAddr(void);

/* 主机单独读取光照强度寄存器 (SLAVE_REG_LIGHT, 2 字节)
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_ReadLightByAddr(void);

#endif
