#include "slave.h"

uint8_t slave_reg[REG_MAX]; /* 从机寄存器数组 */
uint8_t reg_ptr = 0;        /* 当前寄存器指针 */
uint8_t slave_state = SLAVE_STATE_ADDR; /* 从机通信状态 */

/**
 * @brief 从机寄存器初始化
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

    /* 设置默认值 */
    slave_reg[REG_ID]     = 0xAB; /* ID寄存器默认值 0xAB */
    slave_reg[REG_STATUS] = 0x01; /* 状态寄存器：就绪标志 */
    slave_reg[REG_CTRL]   = 0x00; /* 控制寄存器默认值 */

    /* 重置指针和状态 */
    reg_ptr     = 0;
    slave_state = SLAVE_STATE_ADDR;
}
