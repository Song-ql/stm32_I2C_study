#include "slave.h"
#include "BSP_UART.h"
#include "cmsis_os.h"

/* I2C 互斥量: 在 freertos.c 定义, 保护主从恢复路径 */
extern osMutexId g_i2c_mutex;

TimeData_t g_slave_time = {0};
SensorData_t g_slave_sensor = {0};

/* 更新从机模拟传感器数据
 * 使用 HAL_GetTick() 产生随时间变化的伪数据, 模拟真实传感器采样。
 * Slave_ReadByAddr 在主机读取时按需将 g_slave_sensor 序列化为小端字节发送。
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

/* 从机带地址数据处理: 返回处理的寄存器地址, 超时/异常返回 0xFF */
uint8_t Slave_ReadByAddr(void)
{
    static uint16_t s_slave_fail_cnt = 0;
    HAL_StatusTypeDef status;
    uint8_t  reg;

    /* 接收寄存器地址 (1 字节) */
    status = HAL_I2C_Slave_Receive(&hi2c2, &reg, 1, BSP_IIC_TIMEOUT);
    if (status != HAL_OK)
    {
        /* 连续超时达阈值: I2C2 外设锁死, 执行总线恢复 (DeInit+9 脉冲+ReInit)
         * 注: 从机正常运行 500ms 内必被主机寻址一次, 连续 3 次超时 (3s) 即异常
         * 持有 I2C 互斥量恢复: 避免恢复期间主机发起事务导致连锁恢复
         */
        if (++s_slave_fail_cnt >= BSP_IIC_RECOVER_THRESHOLD)
        {
            s_slave_fail_cnt = 0;
            /* mutex==NULL (创建失败) 时跳过锁保护直接恢复, 否则持锁恢复避免与主机并发 */
            if ((g_i2c_mutex == NULL) || (osMutexWait(g_i2c_mutex, BSP_IIC_MUTEX_TIMEOUT) == osOK))
            {
                printf("[Slave] I2C2 locked, recovering...\r\n");
                BSP_IIC_Recover(&hi2c2, GPIOB, GPIO_PIN_10, GPIO_PIN_11, MX_I2C2_Init);
                if (g_i2c_mutex != NULL)
                {
                    osMutexRelease(g_i2c_mutex);
                }
            }
            /* take 失败说明主机正在恢复, 本轮放弃, 下一轮 reg 接收重新累计 */
        }
        return 0xFF;
    }

    /* 成功接收 reg, 复位失败计数 (后续 case 内的失败由下一轮 reg 接收累计) */
    s_slave_fail_cnt = 0;

    switch (reg)
    {
        case SLAVE_REG_TIME:
        {
            /* 接收时间数据 (7 字节) 并解析到 g_slave_time */
            uint8_t buf[I2C_FRAME_TIME_SIZE];
            status = HAL_I2C_Slave_Receive(&hi2c2, buf, I2C_FRAME_TIME_SIZE, BSP_IIC_TIMEOUT);
            if (status != HAL_OK)
            {
                return 0xFF;
            }
            Slave_ParseTime(buf, &g_slave_time);
            return SLAVE_REG_TIME;
        }
        case SLAVE_REG_TEMP:
        {
            /* 按需序列化温度 (2 字节, 小端) 后发送 */
            uint8_t buf[I2C_SENSOR_SIZE];
            buf[0] = (uint8_t)(g_slave_sensor.temperature & 0xFF);
            buf[1] = (uint8_t)(g_slave_sensor.temperature >> 8);
            status = HAL_I2C_Slave_Transmit(&hi2c2, buf, I2C_SENSOR_SIZE, BSP_IIC_TIMEOUT);
            return (status == HAL_OK) ? SLAVE_REG_TEMP : 0xFF;
        }
        case SLAVE_REG_HUMI:
        {
            uint8_t buf[I2C_SENSOR_SIZE];
            buf[0] = (uint8_t)(g_slave_sensor.humidity & 0xFF);
            buf[1] = (uint8_t)(g_slave_sensor.humidity >> 8);
            status = HAL_I2C_Slave_Transmit(&hi2c2, buf, I2C_SENSOR_SIZE, BSP_IIC_TIMEOUT);
            return (status == HAL_OK) ? SLAVE_REG_HUMI : 0xFF;
        }
        case SLAVE_REG_LIGHT:
        {
            uint8_t buf[I2C_SENSOR_SIZE];
            buf[0] = (uint8_t)(g_slave_sensor.light & 0xFF);
            buf[1] = (uint8_t)(g_slave_sensor.light >> 8);
            status = HAL_I2C_Slave_Transmit(&hi2c2, buf, I2C_SENSOR_SIZE, BSP_IIC_TIMEOUT);
            return (status == HAL_OK) ? SLAVE_REG_LIGHT : 0xFF;
        }
        default:
            return 0xFF;
    }
}
