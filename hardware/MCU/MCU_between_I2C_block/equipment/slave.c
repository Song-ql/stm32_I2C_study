#include "slave.h"
#include "BSP_UART.h"

/* 从机数据缓冲区 */
SlaveBuffer_t g_slave_buffer;

/* 从机时间数据 */
TimeData_t g_slave_time = {0};

/* 从机传感器数据 */
SensorData_t g_slave_sensor = {250, 500, 300};

/* 从机阻塞接收主机写入的时间数据
 * 返回: 0 = 成功, 1 = 失败/超时
 * 注意: 该函数会阻塞直到主机发起写事务或超时。
 */
uint8_t Slave_ReceiveTime(void)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Slave_Receive(&hi2c2, g_slave_buffer.rx_buf,
                                   sizeof(g_slave_buffer.rx_buf),
                                   BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}

/* 从机阻塞发送传感器数据给主机
 * 返回: 0 = 成功, 1 = 失败/超时
 * 注意: 该函数会阻塞直到主机发起读事务或超时。
 *       调用前应确保 g_slave_buffer.tx_buf 已更新。
 */
uint8_t Slave_TransmitSensor(void)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Slave_Transmit(&hi2c2, g_slave_buffer.tx_buf,
                                    sizeof(g_slave_buffer.tx_buf),
                                    BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}

/* 更新从机模拟传感器数据, 并打包到 tx_buf 供主机读取
 * 使用 HAL_GetTick() 产生随时间变化的伪数据, 模拟真实传感器采样。
 */
void Slave_UpdateSensor(void)
{
    /* I2C 正在向主机发送 tx_buf 时跳过本次更新, 避免传输中改写数据 */
    if (hi2c2.State == HAL_I2C_STATE_BUSY_TX)
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
