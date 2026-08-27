#include "BSP_IIC.h"
#include "i2c.h"
#include "slave.h"
#include "master.h"

/* 从机接收完成标志 (供应用层轮询) */
volatile uint8_t g_slave_rx_done = 0;

/* HAL 库回调: I2C1 主机中断接收完成
 * 解析 6 字节传感器数据 (小端序) 到 g_master_sensor, 并置位完成标志。
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        g_master_sensor.temperature = (int16_t)(g_master_buffer.rx_buf[0]
                                             | ((uint16_t)g_master_buffer.rx_buf[1] << 8));
        g_master_sensor.humidity    = (int16_t)(g_master_buffer.rx_buf[2]
                                             | ((uint16_t)g_master_buffer.rx_buf[3] << 8));
        g_master_sensor.light       = (int16_t)(g_master_buffer.rx_buf[4]
                                             | ((uint16_t)g_master_buffer.rx_buf[5] << 8));

        g_master_rx_done = 1;
    }
}

/* HAL 库回调: I2C2 从机接收完成
 * 根据命令字分别处理:
 *   I2C_CMD_SET_TIME  -> 解析时间, 重新进入接收
 *   I2C_CMD_GET_SENSOR-> 打包传感器数据, 进入发送状态 (发送完成回调中再回到接收)
 *   其他              -> 重新进入接收
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        uint8_t cmd = g_slave_buffer.rx_buf[0];

        g_slave_rx_done = 1;

        if (cmd == I2C_CMD_SET_TIME)
        {
            Slave_ParseTime(g_slave_buffer.rx_buf);
            HAL_I2C_Slave_Receive_IT(hi2c, g_slave_buffer.rx_buf, sizeof(g_slave_buffer.rx_buf));
        }
        else if (cmd == I2C_CMD_GET_SENSOR)
        {
            Slave_PackSensor(g_slave_buffer.tx_buf);
            HAL_I2C_Slave_Transmit_IT(hi2c, g_slave_buffer.tx_buf, sizeof(g_slave_buffer.tx_buf));
        }
        else
        {
            HAL_I2C_Slave_Receive_IT(hi2c, g_slave_buffer.rx_buf, sizeof(g_slave_buffer.rx_buf));
        }
    }
}

/* HAL 库回调: I2C2 从机发送完成
 * 主机读取传感器数据结束后, 从机重新回到接收监听状态。
 */
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        HAL_I2C_Slave_Receive_IT(hi2c, g_slave_buffer.rx_buf, sizeof(g_slave_buffer.rx_buf));
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
        g_slave_rx_done = 1;
        HAL_I2C_Slave_Receive_IT(&hi2c2, g_slave_buffer.rx_buf, sizeof(g_slave_buffer.rx_buf));
    }
    else if (hi2c->Instance == I2C1)
    {
        /* 主机接收出错 (AF/BERR/OVR 等), 置位完成标志避免任务一直等待 */
        g_master_rx_done = 1;
    }
}
