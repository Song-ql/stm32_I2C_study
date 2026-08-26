#include "BSP_IIC.h"
#include "i2c.h"
#include "slave.h"

/* 从机接收完成标志 (供应用层轮询) */
volatile uint8_t g_slave_rx_done = 0U;

/* HAL 库回调: I2C2 从机接收完成
 * 一次接收结束后 HAL 将 State 置回 READY, 需重新调用
 * HAL_I2C_Slave_Receive_IT 才能响应下次主机寻址。
 *
 * 必须用非 Seq 版本: Seq 版本要求 State==LISTEN, 此处 State==READY 会返回 HAL_BUSY。
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        g_slave_rx_done = 1U;
        HAL_I2C_Slave_Receive_IT(hi2c,g_slave_buffer.pRx_buf,sizeof(g_slave_buffer.pRx_buf));
    }
}

/* HAL 库回调: I2C 错误 (AF/BERR/OVR 等)
 * 出错时 HAL 会将 hi2c2.State 置回 READY, 但不会自动重启接收。
 *
 * 注意: 主机发送字节数 < rx_size 时, 从机收不到预期字节数会产生 AF,
 *       HAL 走 ErrorCallback 路径 (而非 SlaveRxCpltCallback)。
 *       此时数据已实际写入缓冲区, 需设置 done 标志让应用层感知,
 *       并重启接收以恢复对下次主机寻址的响应能力。
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        g_slave_rx_done = 1U;
        HAL_I2C_Slave_Receive_IT(&hi2c2,g_slave_buffer.pRx_buf,sizeof(g_slave_buffer.pRx_buf));
    }
}
