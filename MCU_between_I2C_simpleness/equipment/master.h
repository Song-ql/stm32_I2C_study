#ifndef MASTER_H
#define MASTER_H

#include "main.h"
#include "BSP_IIC.h"

#define BSP_MASTER_IIC_BUF_SIZE 2

/* 主机发送缓冲区 */
typedef struct
{
    uint8_t tx_buf[BSP_MASTER_IIC_BUF_SIZE];
} MasterBuffer_t;

extern MasterBuffer_t g_master_buffer;

/* I2C1 主机发送数据 (阻塞方式)
 * 参数: pData - 数据指针; Size - 数据长度
 * 返回: 0 = 成功, 1 = 失败
 */
uint8_t Master_SendData(void);

#endif
