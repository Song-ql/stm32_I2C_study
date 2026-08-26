#ifndef SLAVE_H
#define SLAVE_H

#include "main.h"
#include "BSP_IIC.h"
#include "i2c.h"

/* 接收缓冲区大小 */
#define BSP_SLAVE_IIC_BUF_SIZE          2

/* 从机接收缓冲区 (直接内嵌, 无需外部数组) */
typedef struct
{
    uint8_t pRx_buf[BSP_SLAVE_IIC_BUF_SIZE];
} SlaveBuffer_t;

extern SlaveBuffer_t g_slave_buffer;


#endif
