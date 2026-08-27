#include "master.h"
#include "i2c.h"
#include <string.h>

MasterBuffer_t g_master_buffer;

volatile uint8_t g_master_rx_done = 0;

SensorData_t g_master_sensor;

/* 将时间数据打包到 tx_buf (小端序)
 * 帧格式: [cmd][year_lo][year_hi][month][day][hour][minute][second]
 */
static void Master_PackTime(uint8_t *buf, const TimeData_t *pTime)
{
    buf[0] = I2C_CMD_SET_TIME;
    buf[1] = (uint8_t)(pTime->year & 0xFF);
    buf[2] = (uint8_t)((pTime->year >> 8) & 0xFF);
    buf[3] = pTime->month;
    buf[4] = pTime->day;
    buf[5] = pTime->hour;
    buf[6] = pTime->minute;
    buf[7] = pTime->second;
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

/* 主机启动读取从机传感器数据
 * 流程: 阻塞发送 GET_SENSOR 命令 (8 字节), 然后以中断方式启动接收 6 字节。
 * 返回: 0 = 成功启动, 1 = 失败
 * 接收完成后由 HAL_I2C_MasterRxCpltCallback 解析数据到 g_master_sensor,
 * 并置位 g_master_rx_done, 调用方轮询该标志读取结果。
 */
uint8_t Master_ReadSensor(void)
{
    HAL_StatusTypeDef status;

    /* 发送读取命令 (阻塞) */
    memset(g_master_buffer.tx_buf, 0, sizeof(g_master_buffer.tx_buf));
    g_master_buffer.tx_buf[0] = I2C_CMD_GET_SENSOR;

    status = HAL_I2C_Master_Transmit(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                                     g_master_buffer.tx_buf,
                                     sizeof(g_master_buffer.tx_buf),
                                     BSP_IIC_TIMEOUT);
    if (status != HAL_OK)
    {
        return 1;
    }

    /* 以中断方式启动接收, 接收完成后进入 MasterRxCpltCallback */
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

