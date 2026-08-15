#include "master.h"

uint8_t id;       /* 主机读取的从机ID */
uint8_t temp[2];  /* 温度数据缓冲区 [高8位, 低8位] */
uint8_t hum[2];   /* 湿度数据缓冲区 [高8位, 低8位] */

/**
 * @brief 向从机写入单个寄存器
 * @param reg   寄存器地址 (使用 SlaveReg_t 枚举)
 * @param value 要写入的值
 * @retval None
 *
 * 通信时序:
 *   S + ADDR(W) + REG_ADDR + DATA + P
 */
void Slave_WriteReg(uint8_t reg, uint8_t value)
{
    HAL_I2C_Mem_Write(&hi2c1,
                      SLAVE_ADDR << 1,
                      reg,
                      I2C_MEMADD_SIZE_8BIT,
                      &value,
                      1,
                      HAL_MAX_DELAY);
}

/**
 * @brief 从从机读取单个寄存器
 * @param reg  寄存器地址 (使用 SlaveReg_t 枚举)
 * @return uint8_t 读取到的值
 *
 * 通信时序:
 *   S + ADDR(W) + REG_ADDR + Sr + ADDR(R) + DATA + P
 */
uint8_t Slave_ReadReg(uint8_t reg)
{
    uint8_t value;
    HAL_I2C_Mem_Read(&hi2c1,
                     SLAVE_ADDR << 1,
                     reg,
                     I2C_MEMADD_SIZE_8BIT,
                     &value,
                     1,
                     HAL_MAX_DELAY);
    return value;
}

/**
 * @brief 从从机连续读取多个寄存器
 * @param start_reg 起始寄存器地址
 * @param buf       存储读取到的值的缓冲区指针
 * @param len       要读取的寄存器数量
 * @retval None
 *
 * 通信时序:
 *   S + ADDR(W) + START_REG + Sr + ADDR(R) + DATA0 + DATA1 + ... + P
 * 从机收到首地址后自动递增指针，连续返回数据
 */
void Slave_ReadRegs(uint8_t start_reg, uint8_t *buf, uint8_t len)
{
    HAL_I2C_Mem_Read(&hi2c1,
                     SLAVE_ADDR << 1,
                     start_reg,
                     I2C_MEMADD_SIZE_8BIT,
                     buf,
                     len,
                     HAL_MAX_DELAY);
}

/* ============================================================
 * 高级功能API：基于基础API封装的业务函数
 * ============================================================ */

/**
 * @brief 读取从机ID并校验
 * @return uint8_t 读取到的ID值，0xFF表示通信失败
 */
uint8_t Master_ReadID(void)
{
    id = Slave_ReadReg(REG_ID);
    /* 从机初始化时设置 ID = 0x5A，这里可以做校验 */
    return id;
}

/**
 * @brief 读取温度值（16位）
 * @return uint16_t 温度原始值 (REG_TEMP_H << 8 | REG_TEMP_L)
 *
 * 使用连续读一次性读取高8位和低8位，
 * 避免两次单读之间数据被更新导致高低字节不匹配
 */
uint16_t Master_ReadTemperature(void)
{
    Slave_ReadRegs(REG_TEMP_H, temp, 2);
    return (uint16_t)((temp[0] << 8) | temp[1]);
}

/**
 * @brief 读取湿度值（16位）
 * @return uint16_t 湿度原始值 (REG_HUM_H << 8 | REG_HUM_L)
 */
uint16_t Master_ReadHumidity(void)
{
    Slave_ReadRegs(REG_HUM_H, hum, 2);
    return (uint16_t)((hum[0] << 8) | hum[1]);
}

/**
 * @brief 写控制寄存器
 * @param value 控制字
 */
void Master_WriteCtrl(uint8_t value)
{
    Slave_WriteReg(REG_CTRL, value);
}

/**
 * @brief 读状态寄存器
 * @return uint8_t 状态值
 */
uint8_t Master_ReadStatus(void)
{
    return Slave_ReadReg(REG_STATUS);
}

/* ============================================================
 * 综合测试示例：演示如何按寄存器地址进行读写操作
 * ============================================================ */

/**
 * @brief 主机读写综合测试函数
 *        在主循环中调用此函数即可测试完整读写流程
 * @retval None
 */
void Master_Test(void)
{
    uint8_t  status;
    uint16_t temperature;
    uint16_t humidity;

    /* ---------- 1. 读取从机ID，确认通信正常 ---------- */
    id = Master_ReadID();
    if (id != 0xAB)
    {
        /* ID不匹配，通信异常处理 */
        return;
    }

    /* ---------- 2. 读取状态寄存器 ---------- */
    status = Master_ReadStatus();
    if ((status & 0x01) == 0x00)
    {
        /* 从机未就绪 */
        return;
    }

    /* ---------- 3. 写控制寄存器：启动一次测量 ---------- */
    Master_WriteCtrl(0x01); /* bit0 = 1 启动测量 */

    /* 等待测量完成（实际项目中可以用中断或超时判断） */
    HAL_Delay(10);

    /* ---------- 4. 按寄存器地址单独读取 ---------- */
    /* 方式一：逐个读取单寄存器 */
    temp[0] = Slave_ReadReg(REG_TEMP_H);
    temp[1] = Slave_ReadReg(REG_TEMP_L);
    temperature = (uint16_t)((temp[0] << 8) | temp[1]);

    /* ---------- 5. 按起始地址连续读取 ---------- */
    /* 方式二：从 REG_HUM_H 开始连续读 2 个寄存器 */
    Slave_ReadRegs(REG_HUM_H, hum, 2);
    humidity = (uint16_t)((hum[0] << 8) | hum[1]);

    /* ---------- 6. 结果使用示例 ---------- */
    /* 这里可以将 temperature 和 humidity 发送到串口、OLED 等 */
    /* 例如：printf("温度=%d, 湿度=%d\r\n", temperature, humidity); */
}
