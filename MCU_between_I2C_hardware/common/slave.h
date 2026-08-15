#ifndef __SLAVE_H__
#define __SLAVE_H__

#include "i2c.h"    

/* 从机寄存器地址 */
typedef enum {
    REG_STATUS   = 0x00, /* 状态寄存器 */
    REG_TEMP_H   = 0x01, /* 温度寄存器高8位 */
    REG_TEMP_L   = 0x02, /* 温度寄存器低8位 */
    REG_HUM_H    = 0x03, /* 湿度寄存器高8位 */
    REG_HUM_L    = 0x04, /* 湿度寄存器低8位 */
    REG_CTRL     = 0x05, /* 控制寄存器 */
    REG_ID       = 0x06, /* ID寄存器 */
} SlaveReg_t;

#define SLAVE_ADDR 0x50 /* 从机地址 */

#define REG_MAX 0x10 /* 最大寄存器地址 */

/* 从机状态：0=等待接收寄存器地址, 1=已收到寄存器地址（可连续读写数据） */
#define SLAVE_STATE_ADDR 0
#define SLAVE_STATE_DATA 1

extern uint8_t slave_reg[REG_MAX]; /* 从机寄存器数组 */
extern uint8_t reg_ptr;            /* 当前寄存器指针 */
extern uint8_t slave_state;        /* 从机通信状态 */

/* 从机寄存器初始化 */
void Slave_Init(void);



#endif
