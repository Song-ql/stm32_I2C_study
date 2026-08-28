#include "BSP_IIC.h"
#include "i2c.h"
#include "slave.h"
#include "master.h"

<<<<<<< HEAD
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
 * 必须用 Slave_Seq_xxx_IT (此时 state 为 LISTEN, 普通版本要求 READY) */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    UNUSED(AddrMatchCode);

    if (hi2c->Instance == I2C2)
    {
        if (TransferDirection == I2C_DIRECTION_TRANSMIT)
        {
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, g_slave_buffer.rx_buf,
                                         sizeof(g_slave_buffer.rx_buf),
                                         I2C_FIRST_AND_LAST_FRAME);
        }
        else
        {
            HAL_I2C_Slave_Seq_Transmit_IT(hi2c, g_slave_buffer.tx_buf,
                                          sizeof(g_slave_buffer.tx_buf),
                                          I2C_FIRST_AND_LAST_FRAME);
        }
    }
}

/* I2C2 接收完成: 只置标志, 时间解析放到任务中 */
=======
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
>>>>>>> origin/develop
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
<<<<<<< HEAD
        g_slave_rx_done = 1;
=======
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
>>>>>>> origin/develop
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
<<<<<<< HEAD
        if ((hi2c->ErrorCode & HAL_I2C_ERROR_AF) == 0U)
        {
            g_slave_i2c_need_recover = 1;
        }
    }
    else if (hi2c->Instance == I2C1)
    {
=======
        g_slave_rx_done = 1;
        HAL_I2C_Slave_Receive_IT(&hi2c2, g_slave_buffer.rx_buf, sizeof(g_slave_buffer.rx_buf));
    }
    else if (hi2c->Instance == I2C1)
    {
        /* 主机接收出错 (AF/BERR/OVR 等), 置位完成标志避免任务一直等待 */
>>>>>>> origin/develop
        g_master_rx_done = 1;
    }
}
