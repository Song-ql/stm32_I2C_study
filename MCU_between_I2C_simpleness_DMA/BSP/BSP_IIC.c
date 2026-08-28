#include "BSP_IIC.h"
#include "i2c.h"
#include "slave.h"
#include "master.h"

/* 从机接收完成标志 */
volatile uint8_t g_slave_rx_done = 0;

/* 非 AF 错误恢复标志 (中断置位, 任务清除) */
volatile uint8_t g_slave_i2c_need_recover = 0;

/* I2C1 主机接收完成: 只置标志, 数据解析放到任务中 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        g_master_rx_done = 1;
    }
}

/* I2C2 地址匹配: 监听模式下根据主机方向启动接收或发送
 * 必须用 Slave_Seq_xxx_DMA (此时 state 为 LISTEN, 普通版本要求 READY) */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    UNUSED(AddrMatchCode);

    if (hi2c->Instance == I2C2)
    {
        if (TransferDirection == I2C_DIRECTION_TRANSMIT)
        {
            HAL_I2C_Slave_Seq_Receive_DMA(hi2c, g_slave_buffer.rx_buf,
                                          sizeof(g_slave_buffer.rx_buf),
                                          I2C_FIRST_AND_LAST_FRAME);
        }
        else
        {
            HAL_I2C_Slave_Seq_Transmit_DMA(hi2c, g_slave_buffer.tx_buf,
                                           sizeof(g_slave_buffer.tx_buf),
                                           I2C_FIRST_AND_LAST_FRAME);
        }
    }
}

/* I2C2 接收完成: 只置标志, 时间解析放到任务中 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        g_slave_rx_done = 1;
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
 *   AF 错误 (主机字节数不匹配): 数据不完整, 丢弃, 由 ListenCpltCallback 恢复监听
 *   其他错误 (BERR/OVR 等): 置恢复标志, 由任务调用 Slave_RecoverI2C 恢复
 *                          (HAL 会在本回调返回后关中断, 无法在中断内直接恢复)
 *   注意: 任何错误都不置 g_slave_rx_done, 只有成功接收才解析
 * 主机: 置 rx_done 以解除任务轮询阻塞
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        if ((hi2c->ErrorCode & HAL_I2C_ERROR_AF) == 0U)
        {
            g_slave_i2c_need_recover = 1;
        }
    }
    else if (hi2c->Instance == I2C1)
    {
        g_master_rx_done = 1;
    }
}
