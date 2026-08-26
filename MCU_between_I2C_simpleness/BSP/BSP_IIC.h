#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "stm32f1xx_hal.h"

/* 8 位地址 (7 位 << 1), 用于 HAL_I2C_Master_Transmit 的 DevAddress 参数。
 * HAL 不会自动左移, 必须传 8 位地址, 否则会寻址到错误的从机。
 */
#define BSP_IIC_SLAVE_ADDR_8BIT   (0x50<<1)

/* I2C 传输超时时间(ms) */
#define BSP_IIC_TIMEOUT           1000

#endif
