#include "BSP_IIC.h"
#include "i2c.h"
#include "slave.h"
#include "master.h"

/* 从机接收完成标志 */
volatile uint8_t g_slave_rx_done = 0;

/* 非 AF 错误恢复标志 (中断置位, 任务清除) */
volatile uint8_t g_slave_i2c_need_recover = 0;

/* 从机接收阶段: 0 = 等待寄存器地址, 1 = 等待数据 */
#define RX_PHASE_ADDR    0
#define RX_PHASE_DATA    1

/* I2C1 主机接收完成: 只置标志, 数据解析放到任务中 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        g_master_rx_done = 1;
    }
}

/* I2C1 主机 Mem 读完成: 寄存器读事务完成回调 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        g_master_rx_done = 1;
    }
}

/* I2C2 地址匹配: 监听模式下根据主机方向启动接收或发送
 * 写方向 (主机发送): 先接收 1 字节寄存器地址, 再由 SlaveRxCpltCallback 继续接收数据
 * 读方向 (主机接收): 根据 g_slave_reg_addr 准备数据并发送
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    UNUSED(AddrMatchCode);

    if (hi2c->Instance == I2C2)
    {
        if (TransferDirection == I2C_DIRECTION_TRANSMIT)
        {
            /* 主机写方向: 先接收 1 字节寄存器地址 (第一帧) */
            g_slave_rx_phase = RX_PHASE_ADDR;
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, g_slave_buffer.rx_buf,
                                         1,
                                         I2C_FIRST_FRAME);
        }
        else
        {
            /* 主机读方向: 根据 g_slave_reg_addr 准备数据并发送
             * 注意: 此时 g_slave_reg_addr 已由前一个写事务 (或上一步地址接收) 设置
             */
            g_slave_rx_phase = RX_PHASE_ADDR;  /* 复位接收阶段 */
            Slave_PrepareTx((uint8_t)g_slave_reg_addr);
            HAL_I2C_Slave_Seq_Transmit_IT(hi2c, g_slave_buffer.tx_buf,
                                          (uint16_t)g_slave_tx_len,
                                          I2C_FIRST_AND_LAST_FRAME);
        }
    }
}

/* I2C2 从机接收完成: 两阶段接收
 * - 阶段0 (地址): 收到寄存器地址, 继续接收数据 (最后一帧)
 * - 阶段1 (数据): 数据接收完成, 置标志由任务处理
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        if (g_slave_rx_phase == RX_PHASE_ADDR)
        {
            /* 收到寄存器地址, 保存后继续接收数据 */
            g_slave_reg_addr = g_slave_buffer.rx_buf[0];
            g_slave_rx_phase = RX_PHASE_DATA;
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, &g_slave_buffer.rx_buf[1],
                                         I2C_MAX_DATA_SIZE,
                                         I2C_LAST_FRAME);
        }
        else
        {
            /* 数据接收完成 (主机发送了满长度数据) */
            g_slave_rx_phase = RX_PHASE_ADDR;
            g_slave_rx_data_len = I2C_MAX_DATA_SIZE;
            g_slave_rx_done = 1;
        }
    }
}

/* I2C2 监听结束: 重新使能监听 */
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        HAL_I2C_EnableListen_IT(hi2c);
    }
}

/* I2C 错误回调
 * 从机:
 *   AF 错误 (主机字节数不匹配):
 *     - 数据接收阶段 (phase=1): 主机发送了 [REG_ADDR][部分数据] 后 STOP,
 *       或主机只发送了 [REG_ADDR] (读指针设置) 后 STOP/ReSTART。
 *       根据 XferCount 计算实际收到的数据长度, >0 则视为合法写操作。
 *     - 地址接收阶段 (phase=0): 异常, 忽略。
 *     任何 AF 错误都不处理 g_slave_rx_done (除上述部分数据情况),
 *     由 ListenCpltCallback 恢复监听。
 *   其他错误 (BERR/OVR 等): 置恢复标志, 由任务调用 Slave_RecoverI2C 恢复。
 * 主机: 置 rx_done 以解除任务轮询阻塞。
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        if ((hi2c->ErrorCode & HAL_I2C_ERROR_AF) != 0U)
        {
            /* AF 错误: 主机发送的字节数与从机请求不匹配 */
            if (g_slave_rx_phase == RX_PHASE_DATA)
            {
                /* 数据接收阶段: 计算实际收到的数据字节数
                 * XferSize = 请求接收数, XferCount = 剩余未接收数
                 * 已接收 = XferSize - XferCount
                 */
                uint16_t received = hi2c->XferSize - hi2c->XferCount;
                if (received > 0U)
                {
                    /* 收到了部分数据, 视为合法写操作 */
                    g_slave_rx_data_len = (uint8_t)received;
                    g_slave_rx_done = 1;
                }
                /* received == 0: 主机只发了 REG_ADDR (读指针设置), 不处理 */
            }
            g_slave_rx_phase = RX_PHASE_ADDR;
            /* AF 错误由 ListenCpltCallback 自动恢复监听 */
        }
        else
        {
            /* 其他错误 (BERR/OVR 等): 置恢复标志 */
            g_slave_i2c_need_recover = 1;
        }
    }
    else if (hi2c->Instance == I2C1)
    {
        g_master_rx_done = 1;
    }
}
