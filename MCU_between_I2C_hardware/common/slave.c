#include "slave.h"

uint8_t slave_reg[REG_MAX]; /* 从机寄存器数组 */
uint8_t reg_ptr = 0;        /* 当前寄存器指针 */
uint8_t slave_state = SLAVE_STATE_ADDR; /* 从机通信状态 */

/**
 * @brief 基于 switch-case 的本地寄存器写函数
 * @param reg_addr 寄存器地址
 * @param value    要写入的值
 * @retval None
 */
void Reg_Write(uint8_t reg_addr, uint8_t value)
{
    switch (reg_addr)
    {   
        case REG_CTRL:
            slave_reg[REG_CTRL] = value;
            break;
        
        default:
            /* 未定义的寄存器地址，可选择忽略或做错误处理 */
            break;
    }
}

/**
 * @brief 基于 switch-case 的本地寄存器读函数
 * @param reg_addr 寄存器地址
 * @retval 寄存器值
 */
uint8_t Reg_Read(uint8_t reg_addr)
{
    switch (reg_addr)
    {
        case REG_STATUS:
            return slave_reg[REG_STATUS];
        case REG_TEMP_H:
            return slave_reg[REG_TEMP_H];
        case REG_TEMP_L:
            return slave_reg[REG_TEMP_L];
        case REG_HUM_H:
            return slave_reg[REG_HUM_H];
        case REG_HUM_L:
            return slave_reg[REG_HUM_L];
        case REG_CTRL:
            return slave_reg[REG_CTRL];
        case REG_ID:
            return slave_reg[REG_ID];
        default:
            /* 未定义的寄存器地址，可选择忽略或做错误处理 */
            return 0x00;
    }
}
/**
 * @brief 获取本地寄存器地址指针（用于 HAL 库连续传输）
 * @param reg_addr 寄存器地址
 * @retval 寄存器地址指针
 */
uint8_t* Reg_GetPtr(uint8_t reg_addr)
{
    switch (reg_addr)
    {
        case REG_STATUS:
            return &slave_reg[REG_STATUS];
        case REG_TEMP_H:
            return &slave_reg[REG_TEMP_H];
        case REG_TEMP_L:
            return &slave_reg[REG_TEMP_L];
        case REG_HUM_H:
            return &slave_reg[REG_HUM_H];
        case REG_HUM_L:
            return &slave_reg[REG_HUM_L];
        case REG_CTRL:
            return &slave_reg[REG_CTRL];
        case REG_ID:
            return &slave_reg[REG_ID];
        default:
            /* 未定义寄存器：返回对应数组位置（初始值0x00），避免污染已定义寄存器 */
            return &slave_reg[reg_addr];
    }
}

/**
 * @brief 从机寄存器初始化（使用 switch-case 设置默认值）
 * @retval None
 */
void Slave_Init(void)
{
    uint8_t i;

    /* 清空所有寄存器 */
    for (i = 0; i < REG_MAX; i++)
    {
        slave_reg[i] = 0x00;
    }

    /* 使用 switch-case 设置默认值 */
    Reg_Write(REG_ID,     0xAB); /* ID寄存器默认值 0xAB */
    Reg_Write(REG_STATUS, 0x01); /* 状态寄存器：就绪标志 */
    Reg_Write(REG_CTRL,   0x00); /* 控制寄存器默认值 */

    /* 重置指针和状态 */
    reg_ptr       = 0;
    slave_state   = SLAVE_STATE_ADDR;
}
