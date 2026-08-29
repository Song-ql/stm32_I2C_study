#include "slave.h"
#include "BSP_UART.h"

TimeData_t g_slave_time = {2026, 1, 1, 0, 0, 0};
SensorData_t g_slave_sensor = {250, 500, 300};

/* 从机寄存器文件: 主机通过寄存器地址 (数据地址) 访问 */
uint8_t g_slave_regs[SLAVE_REG_SIZE];

/* 从机初始化: 阻塞式收发, 不使用监听模式
 * 初始化寄存器文件, 将时间和传感器数据同步到对应寄存器地址。
 */
void Slave_Init(void)
{
    /* 初始化寄存器文件并同步初始数据 */
    for (uint16_t i = 0; i < SLAVE_REG_SIZE; i++)
    {
        g_slave_regs[i] = 0;
    }
    Slave_SyncTimeToRegs();
    Slave_SyncSensorToRegs();
}

/* 从机 I2C 错误恢复 (阻塞式无需恢复监听, 保留为空操作以兼容任务调用)
 * 阻塞式收发中, 超时或错误由 HAL 函数返回值处理, 无需额外恢复。
 */
void Slave_RecoverI2C(void)
{
}

/* 更新从机模拟传感器数据, 并同步到寄存器文件供主机读取
 * 使用 HAL_GetTick() 产生随时间变化的伪数据, 模拟真实传感器采样。
 */
void Slave_UpdateSensor(void)
{
    uint32_t tick;

    /* I2C 正在向主机发送数据时跳过本次更新, 避免传输中改写寄存器数据 */
    if (hi2c2.State == HAL_I2C_STATE_BUSY_TX)
    {
        return;
    }

    tick = HAL_GetTick();

    /* 温度: 20.0 ~ 30.0 °C (x10 -> 200 ~ 300) */
    g_slave_sensor.temperature = (int16_t)(200 + (tick % 101));

    /* 湿度: 40.0 ~ 60.0 % (x10 -> 400 ~ 600) */
    g_slave_sensor.humidity = (int16_t)(400 + ((tick / 7) % 201));

    /* 光照强度: 0 ~ 999 lux */
    g_slave_sensor.light = (int16_t)((tick * 13) % 1000);

    /* 同步到寄存器文件, 供带地址读取 (Mem_Read) 使用 */
    Slave_SyncSensorToRegs();
		printf("[Slave] Send Sensor: Temp=%d.%d C, Humi=%d.%d %%, Light=%d lux\r\n",
					 g_slave_sensor.temperature / 10, g_slave_sensor.temperature % 10,
           g_slave_sensor.humidity / 10, g_slave_sensor.humidity % 10,
           g_slave_sensor.light);
}

/* 将时间数据从 buf 解析到 out (小端序)
 * 格式: [year_lo][year_hi][month][day][hour][minute][second]
 */
void Slave_ParseTime(const uint8_t *buf, TimeData_t *out)
{
    out->year   = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    out->month  = buf[2];
    out->day    = buf[3];
    out->hour   = buf[4];
    out->minute = buf[5];
    out->second = buf[6];
}

/* ===================== 带数据地址的从机接口 ===================== */

/* 将时间数据同步到寄存器文件 (SLAVE_REG_TIME 起始 7 字节) */
void Slave_SyncTimeToRegs(void)
{
    g_slave_regs[SLAVE_REG_TIME + 0] = (uint8_t)(g_slave_time.year & 0xFF);
    g_slave_regs[SLAVE_REG_TIME + 1] = (uint8_t)(g_slave_time.year >> 8);
    g_slave_regs[SLAVE_REG_TIME + 2] = g_slave_time.month;
    g_slave_regs[SLAVE_REG_TIME + 3] = g_slave_time.day;
    g_slave_regs[SLAVE_REG_TIME + 4] = g_slave_time.hour;
    g_slave_regs[SLAVE_REG_TIME + 5] = g_slave_time.minute;
    g_slave_regs[SLAVE_REG_TIME + 6] = g_slave_time.second;
}

/* 将传感器数据同步到寄存器文件
 * 温度/湿度/光照 分别写入各自独立的寄存器地址 (各 2 字节, 小端)
 */
void Slave_SyncSensorToRegs(void)
{
    g_slave_regs[SLAVE_REG_TEMP + 0] = (uint8_t)(g_slave_sensor.temperature & 0xFF);
    g_slave_regs[SLAVE_REG_TEMP + 1] = (uint8_t)(g_slave_sensor.temperature >> 8);

    g_slave_regs[SLAVE_REG_HUMI + 0] = (uint8_t)(g_slave_sensor.humidity & 0xFF);
    g_slave_regs[SLAVE_REG_HUMI + 1] = (uint8_t)(g_slave_sensor.humidity >> 8);

    g_slave_regs[SLAVE_REG_LIGHT + 0] = (uint8_t)(g_slave_sensor.light & 0xFF);
    g_slave_regs[SLAVE_REG_LIGHT + 1] = (uint8_t)(g_slave_sensor.light >> 8);
}

/* 从机带地址接收主机写入的数据
 * 主机使用 HAL_I2C_Mem_Write, 帧结构: [Reg][Data...], 本函数一次接收全部。
 * 本项目中主机写入的是时间数据 (7 字节) 到 SLAVE_REG_TIME,
 * 因此接收总长度 = 1 (寄存器地址) + I2C_FRAME_SIZE (时间数据)。
 */
uint8_t Slave_ReceiveByAddr(void)
{
    HAL_StatusTypeDef status;
    uint8_t  reg;
    uint16_t data_len;
    uint16_t i;
    /* 接收缓冲区: 1 字节寄存器地址 + 最多 SLAVE_REG_SIZE 字节数据 */
    uint8_t  rx_frame[1 + SLAVE_REG_SIZE];

    /* 接收 [寄存器地址 + 时间数据], 共 1 + I2C_FRAME_SIZE 字节 */
    status = HAL_I2C_Slave_Receive(&hi2c2, rx_frame,
                                   1 + I2C_FRAME_SIZE,
                                   BSP_IIC_TIMEOUT);
    if (status != HAL_OK)
    {
        return 1;
    }

    reg      = rx_frame[0];
    data_len = I2C_FRAME_SIZE;

    /* 寄存器范围检查, 防止越界 */
    if ((uint16_t)reg + data_len > SLAVE_REG_SIZE)
    {
        return 1;
    }

    /* 将数据写入寄存器文件 */
    for (i = 0; i < data_len; i++)
    {
        g_slave_regs[reg + i] = rx_frame[1 + i];
    }

    /* 若写入的是时间寄存器, 同步解析到 g_slave_time */
    if (reg == SLAVE_REG_TIME)
    {
        Slave_ParseTime(&g_slave_regs[SLAVE_REG_TIME], &g_slave_time);
    }

    return 0;
}

/* 从机带地址发送数据给主机
 * 主机使用 HAL_I2C_Mem_Read, 时序:
 *   写阶段: S Addr(W) A [Reg] A
 *   读阶段: Sr Addr(R) A [Data0] A [Data1] A P
 * 本函数先阻塞接收 1 字节寄存器地址 (写阶段), 再阻塞发送该地址对应的数据 (读阶段)。
 * 温度/湿度/光照 各自独立寄存器, 每个寄存器返回 2 字节 (I2C_SENSOR_ITEM_SIZE)。
 */
uint8_t Slave_TransmitByAddr(void)
{
    HAL_StatusTypeDef status;
    uint8_t  reg;
    uint16_t data_len = I2C_SENSOR_ITEM_SIZE;

    /* 阶段 1: 接收主机写入的寄存器地址 (1 字节) */
    status = HAL_I2C_Slave_Receive(&hi2c2, &reg, 1, BSP_IIC_TIMEOUT);
    if (status != HAL_OK)
    {
        return 1;
    }

    /* 寄存器范围检查 */
    if ((uint16_t)reg + data_len > SLAVE_REG_SIZE)
    {
        return 1;
    }

    /* 阶段 2: 发送从该寄存器地址开始的数据 (2 字节) */
    status = HAL_I2C_Slave_Transmit(&hi2c2, &g_slave_regs[reg],
                                    data_len, BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}
