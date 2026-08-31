#include "master.h"
#include "i2c.h"

MasterBuffer_t g_master_buffer;

volatile uint8_t g_master_rx_done = 0;

SensorData_t g_master_sensor;

/* 主机向指定寄存器地址写入数据 (阻塞方式)
 * 使用 HAL_I2C_Mem_Write, 总线时序:
 *   [START][SlaveAddr+W][REG_ADDR][DATA...][STOP]
 */
uint8_t Master_WriteReg(uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;

    if (len > I2C_MAX_DATA_SIZE)
    {
        return 1;
    }

    status = HAL_I2C_Mem_Write(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                               (uint16_t)reg_addr, I2C_MEMADD_SIZE_8BIT,
                               (uint8_t *)data, len,
                               BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}

/* 主机从指定寄存器地址读取数据 (中断接收方式)
 * 使用 HAL_I2C_Mem_Read_IT, 总线时序:
 *   [START][SlaveAddr+W][REG_ADDR][Sr][SlaveAddr+R][DATA...][STOP]
 * 接收完成后由 HAL_I2C_MemRxCpltCallback 置位 g_master_rx_done。
 */
uint8_t Master_ReadReg(uint8_t reg_addr, uint16_t len)
{
    HAL_StatusTypeDef status;

    if (len > I2C_MAX_DATA_SIZE)
    {
        return 1;
    }

    g_master_rx_done = 0;
    status = HAL_I2C_Mem_Read_IT(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                                 (uint16_t)reg_addr, I2C_MEMADD_SIZE_8BIT,
                                 g_master_buffer.rx_buf, len);
    if (status != HAL_OK)
    {
        return 1;
    }

    return 0;
}

/* 将时间数据打包到 buf (小端序)
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

/* 主机发送时间到从机 (写 REG_ADDR_TIME, 阻塞方式) */
uint8_t Master_SendTime(const TimeData_t *pTime)
{
    Master_PackTime(g_master_buffer.tx_buf, pTime);

    return Master_WriteReg(REG_ADDR_TIME, g_master_buffer.tx_buf,
                           sizeof(TimeData_t));
}

/* 主机读取从机传感器数据 (读 REG_ADDR_SENSOR, 中断接收)
 * 接收完成后由 HAL_I2C_MemRxCpltCallback 置位 g_master_rx_done,
 * 调用方轮询该标志后调用 Master_ParseSensor 解析数据。
 */
uint8_t Master_ReadSensor(void)
{
    return Master_ReadReg(REG_ADDR_SENSOR, sizeof(SensorData_t));
}

/* 解析接收到的传感器数据 (小端序)
 * 参数: buf - 原始字节缓冲区; out - 解析结果输出
 * 返回: 0 = 成功, out 已更新
 * 应在任务中 g_master_rx_done 置位后调用。
 */
uint8_t Master_ParseSensor(const uint8_t *buf, SensorData_t *out)
{
    out->temperature = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    out->humidity    = (int16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    out->light       = (int16_t)(buf[4] | ((uint16_t)buf[5] << 8));
    return 0;
}
