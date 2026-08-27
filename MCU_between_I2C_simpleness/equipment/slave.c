#include "slave.h"

SlaveBuffer_t g_slave_buffer;

TimeData_t g_slave_time = {2026, 1, 1, 0, 0, 0};
SensorData_t g_slave_sensor = {250, 500, 300};

void Slave_Init(void)
{
    HAL_I2C_Slave_Receive_IT(&hi2c2, g_slave_buffer.rx_buf, sizeof(g_slave_buffer.rx_buf));
}

/* 更新从机模拟传感器数据
 * 使用 HAL_GetTick() 产生随时间变化的伪数据, 模拟真实传感器采样。
 */
void Slave_UpdateSensor(void)
{
    uint32_t tick = HAL_GetTick();

    /* 温度: 20.0 ~ 30.0 °C (x10 -> 200 ~ 300) */
    g_slave_sensor.temperature = (int16_t)(200 + (tick % 101));

    /* 湿度: 40.0 ~ 60.0 % (x10 -> 400 ~ 600) */
    g_slave_sensor.humidity = (int16_t)(400 + ((tick / 7) % 201));

    /* 光照强度: 0 ~ 999 lux */
    g_slave_sensor.light = (int16_t)((tick * 13) % 1000);
}

/* 将接收到的时间帧解析到 g_slave_time
 * 帧格式: [cmd][year_lo][year_hi][month][day][hour][minute][second]
 */
void Slave_ParseTime(const uint8_t *buf)
{
    g_slave_time.year   = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8);
    g_slave_time.month  = buf[3];
    g_slave_time.day    = buf[4];
    g_slave_time.hour   = buf[5];
    g_slave_time.minute = buf[6];
    g_slave_time.second = buf[7];
}

/* 将传感器数据打包到 tx_buf (小端序) */
void Slave_PackSensor(uint8_t *buf)
{
    buf[0] = (uint8_t)(g_slave_sensor.temperature & 0xFF);
    buf[1] = (uint8_t)((g_slave_sensor.temperature >> 8) & 0xFF);
    buf[2] = (uint8_t)(g_slave_sensor.humidity & 0xFF);
    buf[3] = (uint8_t)((g_slave_sensor.humidity >> 8) & 0xFF);
    buf[4] = (uint8_t)(g_slave_sensor.light & 0xFF);
    buf[5] = (uint8_t)((g_slave_sensor.light >> 8) & 0xFF);
}
