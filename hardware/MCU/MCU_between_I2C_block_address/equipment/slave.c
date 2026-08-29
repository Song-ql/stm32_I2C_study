#include "slave.h"
#include "BSP_UART.h"
#include <string.h>

TimeData_t g_slave_time = {0};
SensorData_t g_slave_sensor = {0};

/* 从机接收时间数据缓冲区 (主机写入) */
uint8_t g_slave_recv_time_buf[I2C_FRAME_TIME_SIZE];

/* 从机发送温度数据缓冲区 (主机读取) */
uint8_t g_slave_send_temp_buf[I2C_SENSOR_SIZE];
/* 从机发送湿度数据缓冲区 (主机读取) */
uint8_t g_slave_send_humi_buf[I2C_SENSOR_SIZE];
/* 从机发送光照数据缓冲区 (主机读取) */
uint8_t g_slave_send_light_buf[I2C_SENSOR_SIZE];

/* 从机初始化: 清零所有缓冲区 */
void Slave_Init(void)
{
    memset(g_slave_recv_time_buf, 0, I2C_FRAME_TIME_SIZE);
    memset(g_slave_send_temp_buf, 0, I2C_SENSOR_SIZE);
    memset(g_slave_send_humi_buf, 0, I2C_SENSOR_SIZE);
    memset(g_slave_send_light_buf, 0, I2C_SENSOR_SIZE);
}

/* 更新从机模拟传感器数据, 并同步到发送缓冲区供主机读取
 * 使用 HAL_GetTick() 产生随时间变化的伪数据, 模拟真实传感器采样。
 */
void Slave_UpdateSensor(void)
{
    uint32_t tick;

    /* I2C 正在向主机发送数据时跳过本次更新, 避免传输中改写缓冲区数据 */
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

    /* 同步到各自的发送缓冲区, 供带地址读取 (Mem_Read) 使用 */
    Slave_SyncSensorToRegs(&g_slave_sensor);

    printf("[Slave] Send Sensor: Temp=%d.%d C, Humi=%d.%d %%, Light=%d lux\r\n",
           g_slave_sensor.temperature / 10, g_slave_sensor.temperature % 10,
           g_slave_sensor.humidity / 10, g_slave_sensor.humidity % 10,
           g_slave_sensor.light);
}

/* 将时间数据从 buf 解析到 out (小端序)
 * 格式: [year_lo][year_hi][month][day][hour][minute][second]
 */
static void Slave_ParseTime(const uint8_t *buf, TimeData_t *out)
{
    out->year   = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    out->month  = buf[2];
    out->day    = buf[3];
    out->hour   = buf[4];
    out->minute = buf[5];
    out->second = buf[6];
}

/* 将传感器数据同步到各自的发送缓冲区 (各 2 字节, 小端) */
static void Slave_SyncSensorToRegs(const SensorData_t *sensor)
{
    g_slave_send_temp_buf[0] = (uint8_t)(sensor->temperature & 0xFF);
    g_slave_send_temp_buf[1] = (uint8_t)(sensor->temperature >> 8);

    g_slave_send_humi_buf[0] = (uint8_t)(sensor->humidity & 0xFF);
    g_slave_send_humi_buf[1] = (uint8_t)(sensor->humidity >> 8);

    g_slave_send_light_buf[0] = (uint8_t)(sensor->light & 0xFF);
    g_slave_send_light_buf[1] = (uint8_t)(sensor->light >> 8);
}

/* 从机带地址数据处理 */
uint8_t Slave_ReadByAddr(void)
{
    HAL_StatusTypeDef status;
    uint8_t  reg;

    /* 接收寄存器地址 (1 字节) */
    status = HAL_I2C_Slave_Receive(&hi2c2, &reg, 1, BSP_IIC_TIMEOUT);
    if (status != HAL_OK)
    {
        return 1;
    }

    switch (reg)
    {
        case SLAVE_REG_TIME:
            /* 接收时间数据 (7 字节) */
            status = HAL_I2C_Slave_Receive(&hi2c2, g_slave_recv_time_buf, I2C_FRAME_TIME_SIZE, BSP_IIC_TIMEOUT);
            /* 解析时间到 g_slave_time */
            Slave_ParseTime(g_slave_recv_time_buf, &g_slave_time);
            break;
        case SLAVE_REG_TEMP:
            /* 发送该传感器的 2 字节数据 (小端) */
            status = HAL_I2C_Slave_Transmit(&hi2c2, &g_slave_send_temp_buf[reg], I2C_SENSOR_SIZE, BSP_IIC_TIMEOUT);
            break;
        case SLAVE_REG_HUMI:
            /* 发送该传感器的 2 字节数据 (小端) */
            status = HAL_I2C_Slave_Transmit(&hi2c2, &g_slave_send_humi_buf[reg], I2C_SENSOR_SIZE, BSP_IIC_TIMEOUT);
            break;
        case SLAVE_REG_LIGHT:
            /* 发送该传感器的 2 字节数据 (小端) */
            status = HAL_I2C_Slave_Transmit(&hi2c2, &g_slave_send_light_buf[reg], I2C_SENSOR_SIZE, BSP_IIC_TIMEOUT);
            break;
        default:
            break;
    }

    return (status == HAL_OK) ? 0 : 1;
}
