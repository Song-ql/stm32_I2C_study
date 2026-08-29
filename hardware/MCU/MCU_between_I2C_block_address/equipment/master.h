#ifndef MASTER_H
#define MASTER_H

#include "main.h"
#include "BSP_IIC.h"

/* 主机数据缓冲区 */
typedef struct
{
    uint8_t tx_buf[I2C_FRAME_SIZE]; /* 发送给从机的时间数据 (7 字节) */
} MasterBuffer_t;

/* 主机数据缓冲区 */
extern MasterBuffer_t g_master_buffer;

/* 主机解析后的传感器数据 */
extern SensorData_t g_master_sensor;

/* ===================== 带数据地址的主机接口 ===================== */

/* 主机向从机指定寄存器地址写入数据 (Mem_Write 方式, 首字节为寄存器地址)
 * 参数: reg  - 寄存器地址
 *       data - 待写入数据指针
 *       len  - 数据长度 (不含寄存器地址字节)
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_WriteReg(uint8_t reg, const uint8_t *data, uint16_t len);

/* 主机从从机指定寄存器地址读取数据 (Mem_Read 方式, 先写寄存器地址再读)
 * 参数: reg  - 寄存器地址
 *       buf  - 接收缓冲区指针
 *       len  - 读取长度
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_ReadReg(uint8_t reg, uint8_t *buf, uint16_t len);

/* 主机带地址发送时间到从机 (写入 SLAVE_REG_TIME, 7 字节)
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_SendTimeByAddr(const TimeData_t *pTime);

/* 主机带地址读取从机传感器数据
 * 温度/湿度/光照 分别从 SLAVE_REG_TEMP / SLAVE_REG_HUMI / SLAVE_REG_LIGHT 读取,
 * 结果直接组装到 g_master_sensor。
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_ReadSensorByAddr(void);

#endif
