#ifndef __MASTER_H__
#define __MASTER_H__

#include "i2c.h"
#include "slave.h"


extern uint8_t id;       /* 主机读取的从机ID */
extern uint8_t temp[2];  /* 温度数据缓冲区 [高8位, 低8位] */
extern uint8_t hum[2];   /* 湿度数据缓冲区 [高8位, 低8位] */


/* 基础寄存器读写API */
void    Slave_WriteReg(uint8_t reg, uint8_t value);          /* 写单个寄存器 */
uint8_t Slave_ReadReg(uint8_t reg);                           /* 读单个寄存器 */
void    Slave_ReadRegs(uint8_t start_reg, uint8_t *buf, uint8_t len); /* 连续读多个寄存器 */

/* 高级功能API */
uint8_t  Master_ReadID(void);              /* 读取从机ID并校验 */
uint16_t Master_ReadTemperature(void);     /* 读取温度值（16位） */
uint16_t Master_ReadHumidity(void);        /* 读取湿度值（16位） */
void     Master_WriteCtrl(uint8_t value);  /* 写控制寄存器 */
uint8_t  Master_ReadStatus(void);          /* 读状态寄存器 */

/* 综合测试示例 */
void Master_Test(void);                    /* 主机读写综合测试 */

#endif
