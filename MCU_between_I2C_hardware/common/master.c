#include "master.h"

static MasterData_t master_data;
/**
 * @brief 向从机写入单个寄存器
 * @param reg   寄存器地址 (使用 MasterReg_t 枚举)
 * @param value 要写入的值
 * @retval None
 *
 * 通信时序:
 *   S + ADDR(W) + REG_ADDR + DATA + P
 */
void Master_WriteReg(uint8_t reg, uint8_t value)
{
    HAL_I2C_Mem_Write(&hi2c2,
                      SLAVE_ADDR << 1,
                      reg,
                      I2C_MEMADD_SIZE_8BIT,
                      &value,
                      1,
                      HAL_MAX_DELAY);
}

/**
 * @brief 从从机读取单个寄存器
 * @param reg  寄存器地址 (使用 MasterReg_t 枚举)
 * @return uint8_t 读取到的值
 *
 * 通信时序:
 *   S + ADDR(W) + REG_ADDR + Sr + ADDR(R) + DATA + P
 */
uint8_t Master_ReadReg(uint8_t reg)
{
    uint8_t value;
    HAL_I2C_Mem_Read(&hi2c2,
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
void Master_ReadRegs(uint8_t start_reg, uint8_t *buf, uint8_t len)
{
    HAL_I2C_Mem_Read(&hi2c2,
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
 * @brief 读取从机ID
 * @return uint8_t 读取到的ID值
 *
 * 从机ID寄存器具有自增特性：每次被读取后值递增1
 * 起始值为0xAB，依次返回 0xAB, 0xAC, 0xAD, ... 溢出后回绕到0x00
 */
uint8_t Master_ReadID(void)
{
    int id = 0;
    id = Master_ReadReg(REG_ID);
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
    uint8_t temp[2];
    Master_ReadRegs(REG_TEMP_H, temp, 2);
    return (uint16_t)((temp[0] << 8) | temp[1]);
}

/**
 * @brief 读取湿度值（16位）
 * @return uint16_t 湿度原始值 (REG_HUM_H << 8 | REG_HUM_L)
 */
uint16_t Master_ReadHumidity(void)
{
    uint8_t hum[2];
    Master_ReadRegs(REG_HUM_H, hum, 2);
    return (uint16_t)((hum[0] << 8) | hum[1]);
}

/**
 * @brief 读状态寄存器
 * @return uint8_t 状态值
 */
uint8_t Master_ReadStatus(void)
{
    return Master_ReadReg(REG_STATUS);
}

/**
 * @brief 读控制寄存器
 * @return uint8_t 控制位值
 */
uint8_t Master_ReadCtrl(void)
{
    return Master_ReadReg(REG_CTRL);
}

/**
 * @brief 写控制寄存器
 * @param value 控制位值
 *
 * REG_CTRL 格式:
 *   bit0     : 状态翻转使能 (1=允许翻转, 0=禁止翻转)
 *   bit7~bit1: 翻转周期 = (bit7~bit1) + 1 次循环 (范围: 1 ~ 128)
 *
 * 示例:
 *   Master_WriteCtrl(0x00) → 禁止翻转
 *   Master_WriteCtrl(0x01) → 每 1 次循环翻转 1 次
 *   Master_WriteCtrl(0x13) → 每 10 次循环翻转 1 次
 */
void Master_WriteCtrl(uint8_t value)
{
    Master_WriteReg(REG_CTRL, value);
}

/* ============================================================
 * 数据获取和设置函数
 * ============================================================ */

/**
 * @brief 读取主机数据结构体
 * @return MasterData_t 主机数据结构体
*/
MasterData_t Master_GetData(void)
{
    return master_data;
}

/**
 * @brief 读取当前从机ID
 * @return uint8_t 当前从机ID值
 */
uint8_t Master_GetID(void)
{
    return master_data.id;
}

/**
 * @brief 读取当前温度值（16位）
 * @return uint16_t 当前温度值值
 */
uint16_t Master_GetTemp(void)
{
    return master_data.temp;
}

/**
 * @brief 读取当前湿度值（16位）
 * @return uint16_t 当前湿度值值
 */
uint16_t Master_GetHum(void)
{
    return master_data.hum;
}

/**
 * @brief 读取当前控制位值
 * @return uint8_t 当前控制位值
 */
uint8_t Master_GetCtrl(void)
{
    return master_data.ctrl;
}

/**
 * @brief 读取当前状态值
 * @return uint8_t 当前状态值
 */
uint8_t Master_GetStatus(void)
{
    return master_data.status;
}

/**
 * @brief 设置从机ID
 * @param id 从机ID值
 */
void Master_SetID(uint8_t id)
{
    master_data.id = id;
}

/**
 * @brief 设置温度值（16位）
 * @param temp 温度原始值 (REG_TEMP_H << 8 | REG_TEMP_L)
 */
void Master_SetTemp(uint16_t temp)
{
    master_data.temp = temp;
}

/**
 * @brief 设置湿度值（16位）
 * @param hum 湿度原始值 (REG_HUM_H << 8 | REG_HUM_L)
 */
void Master_SetHum(uint16_t hum)
{
    master_data.hum = hum;
}

/**
 * @brief 设置控制位值
 * @param ctrl 控制位值
 */
void Master_SetCtrl(uint8_t ctrl)
{
    master_data.ctrl = ctrl;
}

/**
 * @brief 设置状态值
 * @param status 状态值
 */
void Master_SetStatus(uint8_t status)
{
    master_data.status = status;
}
