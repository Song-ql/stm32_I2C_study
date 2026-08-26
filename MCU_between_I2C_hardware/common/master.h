#ifndef __MASTER_H__
#define __MASTER_H__

#include "i2c.h"
#include "slave.h"

/**
 * @brief 主机数据结构体
 * @details 用于存储从机ID、温度和湿度数据
 */
typedef struct
{
    uint8_t id;     /* 从机ID(REG_ID) */
    uint16_t temp; /* 温度原始值 (REG_TEMP_H << 8 | REG_TEMP_L) */
    uint16_t hum;  /* 湿度原始值 (REG_HUM_H << 8 | REG_HUM_L) */
    uint8_t ctrl; /* 控制位 (REG_CTRL) */
    uint8_t status; /* 状态位 (REG_STATUS) */
} MasterData_t;

extern uint8_t id;       /* 主机读取的从机ID */
extern uint8_t temp[2];  /* 温度数据缓冲区 [高8位, 低8位] */
extern uint8_t hum[2];   /* 湿度数据缓冲区 [高8位, 低8位] */


/* 基础寄存器读写API */
void    Master_WriteReg(uint8_t reg, uint8_t value);          /* 写单个寄存器 */
uint8_t Master_ReadReg(uint8_t reg);                           /* 读单个寄存器 */
void    Master_ReadRegs(uint8_t start_reg, uint8_t *buf, uint8_t len); /* 连续读多个寄存器 */

/* 高级功能API */
uint8_t  Master_ReadID(void);              /* 读取从机ID */
uint8_t  Master_VerifyIDIncrement(void);  /* 验证ID自增特性 */
uint16_t Master_ReadTemperature(void);     /* 读取温度值（16位） */
uint16_t Master_ReadHumidity(void);        /* 读取湿度值（16位） */
void     Master_WriteCtrl(uint8_t value);  /* 写控制寄存器(bit0=翻转使能,bit7~1=周期) */
uint8_t  Master_ReadStatus(void);          /* 读状态寄存器 */


MasterData_t Master_GetData(void);
uint8_t Master_GetID(void);
uint16_t Master_GetTemp(void);
uint16_t Master_GetHum(void);
uint8_t Master_GetCtrl(void);
uint8_t Master_GetStatus(void); 

void Master_SetID(uint8_t id);
void Master_SetTemp(uint16_t temp);
void Master_SetHum(uint16_t hum);
void Master_SetCtrl(uint8_t ctrl);
void Master_SetStatus(uint8_t status);



#endif
