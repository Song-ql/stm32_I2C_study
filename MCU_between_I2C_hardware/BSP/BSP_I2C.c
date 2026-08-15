#include "BSP_I2C.h"
#include "slave.h"

/**
 * @brief I2C地址匹配回调函数
 * @param hi2c I2C句柄指针
 * @param transferDirection 传输方向
 * @param addrMatchCode 地址匹配码
 * @retval None
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t transferDirection, uint16_t addrMatchCode)
{
    if (hi2c->Instance == I2C1)
    {
        if (transferDirection == I2C_DIRECTION_TRANSMIT) /* 主机发送 → 从机接收 */
        {
            /* 第一次接收的是寄存器地址，存入 reg_ptr */
            slave_state = SLAVE_STATE_ADDR;
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, &reg_ptr, 1, I2C_NEXT_FRAME);
        }
        else /* 主机接收 → 从机发送（读寄存器） */
        {
            /* 读操作时，reg_ptr 已经在之前的写阶段被设置好了 */
            /* 直接发送当前寄存器的值 */
            if (reg_ptr < REG_MAX)
            {
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &slave_reg[reg_ptr], 1, I2C_NEXT_FRAME);
            }
        }
    }
}

/**
 * @brief I2C接收完成回调函数
 * @param hi2c I2C句柄指针
 * @retval None
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (slave_state == SLAVE_STATE_ADDR)
    {
        /* 刚收到寄存器地址，切换到数据状态，准备接收后续数据 */
        slave_state = SLAVE_STATE_DATA;

        /* 边界检查：reg_ptr 超出范围则重置为 0 */
        if (reg_ptr >= REG_MAX)
        {
            reg_ptr = 0;
        }

        /* 准备接收数据到 slave_reg[reg_ptr] */
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, &slave_reg[reg_ptr], 1, I2C_NEXT_FRAME);
    }
    else /* SLAVE_STATE_DATA */
    {
        /* 数据已写入当前寄存器，指针递增，继续接收下一个 */
        if (reg_ptr < REG_MAX - 1)
        {
            reg_ptr++;
        }

        HAL_I2C_Slave_Seq_Receive_IT(hi2c, &slave_reg[reg_ptr], 1, I2C_NEXT_FRAME);
    }
}

/**
 * @brief I2C发送完成回调函数
 * @param hi2c I2C句柄指针
 * @retval None
 */
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    /* 当前寄存器发送完成，指针递增（支持连续读多个寄存器） */
    if (reg_ptr < REG_MAX - 1)
    {
        reg_ptr++;
    }

    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &slave_reg[reg_ptr], 1, I2C_NEXT_FRAME);
}

/**
 * @brief I2C监听完成回调函数（STOP或RESTART后触发）
 * @param hi2c I2C句柄指针
 * @retval None
 */
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    /* 一次通信结束，重置状态，重新开启监听 */
    slave_state = SLAVE_STATE_ADDR;
    HAL_I2C_EnableListen_IT(hi2c);
}
