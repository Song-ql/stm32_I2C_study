#include "slave.h"
#include "BSP_UART.h"

SlaveBuffer_t g_slave_buffer;

TimeData_t g_slave_time = {2026, 1, 1, 0, 0, 0};
SensorData_t g_slave_sensor = {250, 500, 300};
uint8_t g_slave_version = FIRMWARE_VERSION;
volatile uint8_t g_slave_led = 0;

volatile uint8_t g_slave_reg_addr = 0;
volatile uint8_t g_slave_tx_len = 0;
volatile uint8_t g_slave_rx_phase = 0;
volatile uint8_t g_slave_rx_data_len = 0;

/* 寄存器信息表: 索引 = 寄存器地址, 内容 = {数据长度, 访问权限} */
const RegInfo_t g_reg_table[REG_ADDR_MAX] =
{
    [REG_ADDR_TIME]    = {sizeof(TimeData_t),   REG_ACCESS_RW},  /* 7 字节, 读写 */
    [REG_ADDR_SENSOR]  = {sizeof(SensorData_t), REG_ACCESS_RO},  /* 6 字节, 只读 */
    [REG_ADDR_VERSION] = {1,                    REG_ACCESS_RO},  /* 1 字节, 只读 */
    [REG_ADDR_LED]     = {1,                    REG_ACCESS_RW},  /* 1 字节, 读写 */
};

/* 从机初始化: 使能 I2C2 监听模式 (Listen Mode)
 * 监听模式下, 从机持续监听总线, 被主机寻址时触发 HAL_I2C_AddrCallback,
 * 由该回调根据主机读写方向决定启动接收或发送。
 */
void Slave_Init(void)
{
    /* 预打包传感器数据, 确保主机首次读取前 tx_buf 有效 */
    Slave_PackSensor(&g_slave_sensor, g_slave_buffer.tx_buf);
    g_slave_tx_len = g_reg_table[REG_ADDR_SENSOR].size;
    HAL_I2C_EnableListen_IT(&hi2c2);
}

/* 从机 I2C 监听模式错误恢复
 * 在任务上下文中调用, 检查恢复标志并重新使能监听模式。
 * 用于处理 BERR/OVR 等非 AF 错误 (AF 错误由 ListenCpltCallback 恢复)。
 */
void Slave_RecoverI2C(void)
{
    if (g_slave_i2c_need_recover != 0)
    {
        g_slave_i2c_need_recover = 0;
        HAL_I2C_DisableListen_IT(&hi2c2);
        HAL_I2C_EnableListen_IT(&hi2c2);
    }
}

/* 根据寄存器地址准备发送数据到 tx_buf, 并设置 g_slave_tx_len
 * 只读寄存器在读取时返回当前值; 读写寄存器返回最新写入值。
 * 无效地址返回 1 字节 0x00 (避免 HAL_I2C_Slave_Seq_Transmit_IT 因 Size=0 报错)。
 */
void Slave_PrepareTx(uint8_t reg_addr)
{
    if (reg_addr >= REG_ADDR_MAX)
    {
        /* 无效地址: 返回 1 字节 0x00 作为默认响应 */
        g_slave_buffer.tx_buf[0] = 0x00;
        g_slave_tx_len = 1;
        return;
    }

    g_slave_tx_len = g_reg_table[reg_addr].size;

    switch (reg_addr)
    {
        case REG_ADDR_TIME:
            /* 时间数据小端打包: [year_lo][year_hi][month][day][hour][min][sec] */
            g_slave_buffer.tx_buf[0] = (uint8_t)(g_slave_time.year & 0xFF);
            g_slave_buffer.tx_buf[1] = (uint8_t)(g_slave_time.year >> 8);
            g_slave_buffer.tx_buf[2] = g_slave_time.month;
            g_slave_buffer.tx_buf[3] = g_slave_time.day;
            g_slave_buffer.tx_buf[4] = g_slave_time.hour;
            g_slave_buffer.tx_buf[5] = g_slave_time.minute;
            g_slave_buffer.tx_buf[6] = g_slave_time.second;
            break;

        case REG_ADDR_SENSOR:
            Slave_PackSensor(&g_slave_sensor, g_slave_buffer.tx_buf);
            break;

        case REG_ADDR_VERSION:
            g_slave_buffer.tx_buf[0] = g_slave_version;
            break;

        case REG_ADDR_LED:
            g_slave_buffer.tx_buf[0] = (uint8_t)g_slave_led;
            break;

        default:
            g_slave_buffer.tx_buf[0] = 0x00;
            g_slave_tx_len = 1;
            break;
    }
}

/* 处理主机写入的寄存器数据
 * 对只读寄存器忽略写入; 对读写寄存器更新对应变量。
 */
void Slave_ProcessWrite(uint8_t reg_addr, const uint8_t *data, uint8_t len)
{
    if (reg_addr >= REG_ADDR_MAX)
    {
        return;
    }

    /* 只读寄存器不允许写入 */
    if (g_reg_table[reg_addr].access == REG_ACCESS_RO)
    {
        return;
    }

    /* 长度校验: 至少要达到寄存器定义的长度 */
    if (len < g_reg_table[reg_addr].size)
    {
        return;
    }

    switch (reg_addr)
    {
        case REG_ADDR_TIME:
            Slave_ParseTime(data, &g_slave_time);
            break;

        case REG_ADDR_LED:
            g_slave_led = data[0];
            break;

        default:
            break;
    }
}

/* 更新从机模拟传感器数据, 并打包到 tx_buf 供主机读取
 * 使用 HAL_GetTick() 产生随时间变化的伪数据, 模拟真实传感器采样。
 */
void Slave_UpdateSensor(void)
{
    uint32_t tick;

    /* I2C 正在向主机发送 tx_buf 时跳过本次更新, 避免传输中改写数据 */
    if (hi2c2.State == HAL_I2C_STATE_BUSY_TX_LISTEN)
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

    /* 注意: 不直接打包到 tx_buf, 由 Slave_PrepareTx 在主机读事务时实时打包,
     * 确保主机总是读取到最新的传感器数据。
     */
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
