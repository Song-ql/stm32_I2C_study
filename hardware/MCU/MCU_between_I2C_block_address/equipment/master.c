#include "master.h"
#include "i2c.h"
#include "BSP_UART.h"
#include "cmsis_os.h"

/* I2C 互斥量: 在 freertos.c 定义, 保护主从恢复路径 */
extern osMutexId g_i2c_mutex;

MasterBuffer_t g_master_buffer;

SensorData_t g_master_sensor;

/* 主机连续失败计数 (WriteReg/ReadReg 共用), 达阈值触发 I2C1 外设恢复 */
static uint16_t s_master_fail_cnt = 0;

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
static uint8_t Master_WriteReg(uint8_t reg, const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;

    /* 获取 I2C 互斥量: 避免与另一侧恢复并发 (连锁恢复)
     * mutex==NULL (创建失败) 时跳过锁保护, 回退无锁行为避免系统瘫痪 */
    if ((g_i2c_mutex != NULL) && (osMutexWait(g_i2c_mutex, BSP_IIC_MUTEX_TIMEOUT) != osOK))
    {
        return 1;  /* 另一侧正在恢复, 跳过本次事务, 不累计失败 */
    }

    status = HAL_I2C_Mem_Write(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                               reg, I2C_MEMADD_SIZE_8BIT,
                               (uint8_t *)data, len,
                               BSP_IIC_TIMEOUT);

    if (status == HAL_OK)
    {
        s_master_fail_cnt = 0;
        if (g_i2c_mutex != NULL)
        {
            osMutexRelease(g_i2c_mutex);
        }
        return 0;
    }

    /* 连续失败达阈值: I2C1 外设锁死, 执行总线恢复 (持有 mutex, 另一侧 take 阻塞) */
    if (++s_master_fail_cnt >= BSP_IIC_RECOVER_THRESHOLD)
    {
        s_master_fail_cnt = 0;
        printf("[Master] I2C1 locked, recovering...\r\n");
        BSP_IIC_Recover(&hi2c1, GPIOB, GPIO_PIN_6, GPIO_PIN_7, MX_I2C1_Init);
    }
    if (g_i2c_mutex != NULL)
    {
        osMutexRelease(g_i2c_mutex);
    }
    return 1;
}

/* 主机从从机指定寄存器地址读取数据
 * 总线时序: S Addr(W) A [Reg] A Sr Addr(R) A [Data0] A ... P
 * HAL 内部先发寄存器地址, 再重复起始读数据。
 */
static uint8_t Master_ReadReg(uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef status;

    if ((g_i2c_mutex != NULL) && (osMutexWait(g_i2c_mutex, BSP_IIC_MUTEX_TIMEOUT) != osOK))
    {
        return 1;
    }

    status = HAL_I2C_Mem_Read(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT,
                              reg, I2C_MEMADD_SIZE_8BIT,
                              buf, len,
                              BSP_IIC_TIMEOUT);

    if (status == HAL_OK)
    {
        s_master_fail_cnt = 0;
        if (g_i2c_mutex != NULL)
        {
            osMutexRelease(g_i2c_mutex);
        }
        return 0;
    }

    if (++s_master_fail_cnt >= BSP_IIC_RECOVER_THRESHOLD)
    {
        s_master_fail_cnt = 0;
        printf("[Master] I2C1 locked, recovering...\r\n");
        BSP_IIC_Recover(&hi2c1, GPIOB, GPIO_PIN_6, GPIO_PIN_7, MX_I2C1_Init);
    }
    if (g_i2c_mutex != NULL)
    {
        osMutexRelease(g_i2c_mutex);
    }
    return 1;
}

/* 主机带地址发送时间到从机: 将 7 字节时间写入寄存器 SLAVE_REG_TIME */
uint8_t Master_SendTimeByAddr(const TimeData_t *pTime)
{
    uint8_t buf[I2C_FRAME_TIME_SIZE];
    Master_PackTime(buf, pTime);
    return Master_WriteReg(SLAVE_REG_TIME, buf, I2C_FRAME_TIME_SIZE);
}

/* 主机单独读取温度寄存器 (SLAVE_REG_TEMP, 2 字节) */
uint8_t Master_ReadTempByAddr(void)
{
    uint8_t buf[I2C_SENSOR_SIZE];

    if (Master_ReadReg(SLAVE_REG_TEMP, buf, I2C_SENSOR_SIZE) != 0)
    {
        return 1;
    }
    g_master_sensor.temperature = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    return 0;
}

/* 主机单独读取湿度寄存器 (SLAVE_REG_HUMI, 2 字节) */
uint8_t Master_ReadHumiByAddr(void)
{
    uint8_t buf[I2C_SENSOR_SIZE];

    if (Master_ReadReg(SLAVE_REG_HUMI, buf, I2C_SENSOR_SIZE) != 0)
    {
        return 1;
    }
    g_master_sensor.humidity = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    return 0;
}

/* 主机单独读取光照强度寄存器 (SLAVE_REG_LIGHT, 2 字节) */
uint8_t Master_ReadLightByAddr(void)
{
    uint8_t buf[I2C_SENSOR_SIZE];

    if (Master_ReadReg(SLAVE_REG_LIGHT, buf, I2C_SENSOR_SIZE) != 0)
    {
        return 1;
    }
    g_master_sensor.light = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    return 0;
}
