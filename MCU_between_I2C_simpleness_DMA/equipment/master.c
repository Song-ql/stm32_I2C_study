#include "master.h"
#include "i2c.h"

MasterBuffer_t g_master_buffer;

volatile uint8_t g_master_rx_done = 0;

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

/* 主机读取从机传感器数据 (中断接收方式)
 * 接收完成后由 HAL_I2C_MasterRxCpltCallback 置位 g_master_rx_done,
 * 调用方轮询该标志后调用 Master_ParseSensor 解析数据。
 */
uint8_t Master_ReadSensor(void)
{
    HAL_StatusTypeDef status;

    g_master_rx_done = 0;
    status = HAL_I2C_Master_Receive_IT(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                                       g_master_buffer.rx_buf,
                                       sizeof(g_master_buffer.rx_buf));
    if (status != HAL_OK)
    {
        return 1;
    }

    return 0;
}

/* 解析接收到的传感器数据 (小端序)
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 * 应在任务中 g_master_rx_done 置位后调用。
 */
void Master_ParseSensor(const uint8_t *buf, SensorData_t *out)
{
    out->temperature = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    out->humidity    = (int16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    out->light       = (int16_t)(buf[4] | ((uint16_t)buf[5] << 8));
}

