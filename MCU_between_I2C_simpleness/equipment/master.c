#include "master.h"
#include "i2c.h"

MasterBuffer_t g_master_buffer;

/* I2C1 主机发送数据 (阻塞方式)
 * 调用 HAL_I2C_Master_Transmit 向从机地址发送数据。
 * 注意: 必须确保从机已先进入接收监听状态, 否则主机会因无 ACK 而超时失败。
 */
uint8_t Master_SendData(void)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Transmit(&hi2c1, BSP_IIC_SLAVE_ADDR_8BIT, g_master_buffer.tx_buf, sizeof(g_master_buffer.tx_buf), BSP_IIC_TIMEOUT);

    return (status == HAL_OK) ? 0U : 1U;
}

