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

/* ===================== 带数据地址的主机接口 ===================== */

/* 主机向从机指定寄存器地址写入数据
 * 总线时序: S Addr(W) A [Reg] A [Data0] A ... P
 * HAL 内部自动把 reg 作为首字节发出。
 */
uint8_t Master_WriteReg(uint8_t reg, const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                               reg, I2C_MEMADD_SIZE_8BIT,
                               (uint8_t *)data, len,
                               BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}

/* 主机从从机指定寄存器地址读取数据
 * 总线时序: S Addr(W) A [Reg] A Sr Addr(R) A [Data0] A ... P
 * HAL 内部先发寄存器地址, 再重复起始读数据。
 */
uint8_t Master_ReadReg(uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                              reg, I2C_MEMADD_SIZE_8BIT,
                              buf, len,
                              BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0 : 1;
}

/* 主机带地址发送时间到从机: 将 7 字节时间写入寄存器 SLAVE_REG_TIME */
uint8_t Master_SendTimeByAddr(const TimeData_t *pTime)
{
    Master_PackTime(g_master_buffer.tx_buf, pTime);
    return Master_WriteReg(SLAVE_REG_TIME, g_master_buffer.tx_buf, I2C_FRAME_SIZE);
}

/* 主机带地址读取从机传感器数据
 * 温度/湿度/光照 分别位于独立寄存器, 各 2 字节, 因此分三次 Mem_Read 读取,
 * 结果直接组装到 g_master_sensor。
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_ReadSensorByAddr(void)
{
    uint8_t buf[I2C_SENSOR_ITEM_SIZE];

    /* 读取温度寄存器 (SLAVE_REG_TEMP) */
    if (Master_ReadReg(SLAVE_REG_TEMP, buf, I2C_SENSOR_ITEM_SIZE) != 0)
    {
        return 1;
    }
    g_master_sensor.temperature = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));

    /* 读取湿度寄存器 (SLAVE_REG_HUMI) */
    if (Master_ReadReg(SLAVE_REG_HUMI, buf, I2C_SENSOR_ITEM_SIZE) != 0)
    {
        return 1;
    }
    g_master_sensor.humidity = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));

    /* 读取光照强度寄存器 (SLAVE_REG_LIGHT) */
    if (Master_ReadReg(SLAVE_REG_LIGHT, buf, I2C_SENSOR_ITEM_SIZE) != 0)
    {
        return 1;
    }
    g_master_sensor.light = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));

    return 0;
}

