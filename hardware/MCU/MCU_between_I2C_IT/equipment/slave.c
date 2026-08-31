#include "slave.h"
#include "BSP_UART.h"

SlaveBuffer_t g_slave_buffer;

TimeData_t g_slave_time = {2026, 1, 1, 0, 0, 0};
SensorData_t g_slave_sensor = {250, 500, 300};

/* 从机初始化: 使能 I2C2 监听模式 (Listen Mode)
 * 监听模式下, 从机持续监听总线, 被主机寻址时触发 HAL_I2C_AddrCallback,
 * 由该回调根据主机读写方向决定启动接收或发送。
 */
void Slave_Init(void)
{
    /* 预打包传感器数据, 确保主机首次读取前 tx_buf 有效 */
    Slave_PackSensor(&g_slave_sensor, g_slave_buffer.tx_buf);
    HAL_I2C_EnableListen_IT(&hi2c2);
}

/* 从机 I2C 监听模式错误恢复
 * 在任务上下文中调用, 检查恢复标志并重新使能监听模式。
 * 用于处理 BERR/OVR 等非 AF 错误 (AF 错误由 ListenCpltCallback 恢复)。
 */
void Slave_RecoverI2C(void)
{
    if (g_slave_i2c_need_recover != 0)
    {
        g_slave_i2c_need_recover = 0;
        HAL_I2C_DisableListen_IT(&hi2c2);
        HAL_I2C_EnableListen_IT(&hi2c2);
    }
}

/* 更新从机模拟传感器数据, 并打包到 tx_buf 供主机读取
 * 使用 HAL_GetTick() 产生随时间变化的伪数据, 模拟真实传感器采样。
 */
void Slave_UpdateSensor(void)
{
    /* I2C 正在向主机发送 tx_buf 时跳过本次更新, 避免传输中改写数据 */
    if (hi2c2.State == HAL_I2C_STATE_BUSY_TX_LISTEN)
    {
        return;
    }

    /* 温度 */
    g_slave_sensor.temperature = 200;

    /* 湿度 */
    g_slave_sensor.humidity = 400;

    /* 光照强度 */
    g_slave_sensor.light = 1000;

    /* 打包到发送缓冲区, 供主机读事务直接发送 */
    Slave_PackSensor(&g_slave_sensor, g_slave_buffer.tx_buf);
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

/* 将传感器数据从 sensor 打包到 buf (小端序)
 * 格式: [T_lo][T_hi][H_lo][H_hi][L_lo][L_hi]
 */
void Slave_PackSensor(const SensorData_t *sensor, uint8_t *buf)
{
    buf[0] = (uint8_t)(sensor->temperature & 0xFF);
    buf[1] = (uint8_t)(sensor->temperature >> 8);
    buf[2] = (uint8_t)(sensor->humidity & 0xFF);
    buf[3] = (uint8_t)(sensor->humidity >> 8);
    buf[4] = (uint8_t)(sensor->light & 0xFF);
    buf[5] = (uint8_t)(sensor->light >> 8);
}
