#include "master.h"
#include "i2c.h"

MasterBuffer_t g_master_buffer;

SensorData_t g_master_sensor;

/* 将时间数据打包到 tx_buf (小端序)
 * 格式: [year_lo][year_hi][month][day][hour][minute][second]
 */
static void Master_PackTime(uint8_t *buf, const TimeData_t *pTime)
{
    buf[0] = (uint8_t)(pTime->year & 0xFF);
    buf[1] = (uint8_t)(pTime->year >> 8);
    buf[2] = pTime->month;
    buf[3] = pTime->day;
    buf[4] = pTime->hour;
    buf[5] = pTime->minute;
    buf[6] = pTime->second;
}

/* 主机发送时间到从机 (阻塞方式) */
uint8_t Master_SendTime(const TimeData_t *pTime)
{
    HAL_StatusTypeDef status;

    Master_PackTime(g_master_buffer.tx_buf, pTime);

    status = HAL_I2C_Master_Transmit(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                                     g_master_buffer.tx_buf,
                                     sizeof(g_master_buffer.tx_buf),
                                     BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}

/* 主机读取从机传感器数据 (阻塞接收方式)
 * 函数返回时数据已就绪, 可直接调用 Master_ParseSensor 解析。
 */
uint8_t Master_ReadSensor(void)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Receive(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                                    g_master_buffer.rx_buf,
                                    sizeof(g_master_buffer.rx_buf),
                                    BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}

/* 解析接收到的传感器数据 (小端序)
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 * 返回: 0 = 成功, out 已更新
 * 注意: 在 Master_ReadSensor 成功返回后直接调用即可。
 */
uint8_t Master_ParseSensor(const uint8_t *buf, SensorData_t *out)
{
    out->temperature = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    out->humidity    = (int16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    out->light       = (int16_t)(buf[4] | ((uint16_t)buf[5] << 8));
    return 0;
}

