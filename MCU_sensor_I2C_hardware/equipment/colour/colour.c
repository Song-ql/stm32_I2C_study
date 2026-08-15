#include "colour.h"

/* TCS34725设备地址 */
#define TCS34725_ADDR       0x29 // TCS34725地址

/* TCS34725发送重复协议 */
#define TCS34725_CMD        0x80 // TCS34725发送重复协议

/* TCS34725发送自动递增协议 */
#define TCS34725_CMD_AUTO   (0x80 | 0x20) // TCS34725发送自动递增协议

/* TCS34725寄存器地址 */
#define REG_ENABLE          0x00 // TCS34725使能寄存器
#define REG_ATIME           0x01 // TCS34725积分时间寄存器
#define REG_CONTROL         0x0F // TCS34725色条增益控制寄存器
#define REG_ID              0x12 // TCS34725ID寄存器
#define REG_STATUS          0x13 // TCS34725状态寄存器
#define REG_CDATAL          0x14 // TCS34725数据低字节寄存器

/* TCS34725使能寄存器位 */
#define PON                 0x01 // TCS34725开启电源
#define AEN                 0x02 // TCS34725激活双通道ADC

/* TCS34725积分时间寄存器值 */
#define ATIME               0xD6 // TCS34725积分时间寄存器值

/* TCS34725色条增益控制寄存器位 */
#define CONTROL             0x02 // TCS34725色条4x增益

/* TCS34725设备ID值 */
#define ID_VAL              0x44 // TCS34725ID值

typedef enum {
    TCS_STA_INIT = 0,    // 未初始化:开机首次状态,待执行TCS34725_Init
    TCS_STA_RUNNING,     // 运行中:正常读取RGB,失败3次转RETRY
    TCS_STA_RETRY,       // 重试:重新Init,失败转BUS_RESET
    TCS_STA_BUS_RESET,   // 总线复位:复位I2C后Init,失败转SOFT_RESET
    TCS_STA_SOFT_RESET,  // 软复位:复位传感器后Init,失败转DEGRADED
    TCS_STA_DEGRADED     // 降级:停止读取,每5秒退回RETRY重试
} TCSState;

/* TCS34725 读取结果 */
typedef enum {
    TCS_READ_OK = 0,        // 读取成功
    TCS_READ_NOT_READY,     // 积分未完成,下次再试(不计故障)
    TCS_READ_ERROR          // 通信故障,计入fail_cnt
} TCS_ReadResult;

static TCSState color_sensor_state = TCS_STA_INIT;  // 开机未初始化
static uint8_t color_available    = 0;
static ColorValue color;

/* 降级后重试间隔(ms),避免一次失败永久报废 */
#define DEGRADED_RETRY_INTERVAL_MS  5000

/*********************************************** 内部函数 ***********************************************/
/**
 * @brief 写入TCS34725寄存器
 * @param reg 寄存器地址
 * @param val 要写入的值
 * @return HAL_StatusTypeDef HAL_OK 成功其他错误码
 */
static inline HAL_StatusTypeDef TCS_Write(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(&hi2c1, TCS34725_ADDR << 1,
                             TCS34725_CMD | reg, 1, &val, 1, 30);
}

/**
 * @brief 读取单个TCS34725寄存器
 * @param reg 寄存器地址
 * @param buf 读取的值缓冲区
 * @return HAL_StatusTypeDef HAL_OK 成功其他错误码
 */
static inline HAL_StatusTypeDef TCS_Read(uint8_t reg, uint8_t *buf)
{
    return HAL_I2C_Mem_Read(&hi2c1, TCS34725_ADDR << 1,
                            TCS34725_CMD | reg, 1, buf, 1, 30);
}

/**
 * @brief 读取多个TCS34725寄存器
 * @param reg 寄存器地址
 * @param buf 读取的值缓冲区
 * @param len 读取的字节数
 * @return HAL_StatusTypeDef HAL_OK 成功其他错误码
 */
static inline HAL_StatusTypeDef TCS_Read_Multi(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1, TCS34725_ADDR << 1,
                            TCS34725_CMD_AUTO | reg, 1, buf, len, 30);
}

/**
 * @brief 恢复I2C总线
 */
static void I2C_Bus_Recover(void)
{
    __HAL_I2C_DISABLE(&hi2c1);
    HAL_I2C_DeInit(&hi2c1);
    HAL_I2C_Init(&hi2c1);
    __HAL_I2C_ENABLE(&hi2c1);
}

/**
 * @brief 初始化TCS34725传感器
 * @return uint8_t 初始化成功返回1，否则返回0
 */
static uint8_t TCS34725_Init(void)
{
    uint8_t id = 0;

    if (TCS_Read(REG_ID, &id) != HAL_OK)
        return 0;

    if (id != ID_VAL)
        return 0;

    if (TCS_Write(REG_ENABLE, PON) != HAL_OK)
        return 0;

    HAL_Delay(3);

    if (TCS_Write(REG_ENABLE, AEN | PON) != HAL_OK)
        return 0;

    TCS_Write(REG_ATIME, ATIME);
    TCS_Write(REG_CONTROL, CONTROL);

    return 1;
}

/**
 * @brief 读取TCS34725传感器的RGB值(非阻塞)
 * @param red 红色指针
 * @param green 绿色指针
 * @param blue 蓝色指针
 * @param clear 非色指针
 * @return TCS_ReadResult 读取结果
 * @note  非阻塞:积分未完成返回NOT_READY,不计故障;
 *        clear==0(暗光)和clear==0xFFFF(饱和)均为合法读数
 */
static TCS_ReadResult TCS_Read_RGB(uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t *clear)
{
    uint8_t status;
    uint8_t buf[8];

    /* 非阻塞查询AVALID位 */
    if (TCS_Read(REG_STATUS, &status) != HAL_OK)
        return TCS_READ_ERROR;

    if (!(status & 0x01))
        return TCS_READ_NOT_READY;   /* 积分未完成,下次再试 */

    if (TCS_Read_Multi(REG_CDATAL, buf, 8) != HAL_OK)
        return TCS_READ_ERROR;

    *clear = buf[0] | (buf[1] << 8);
    *red   = buf[2] | (buf[3] << 8);
    *green = buf[4] | (buf[5] << 8);
    *blue  = buf[6] | (buf[7] << 8);

    return TCS_READ_OK;
}

/*********************************************** 外部函数 ***********************************************/
/**
 * @brief TCS34725传感器运行状态机
 */
void TCS34725_Run(void)
{
    static uint32_t fail_cnt = 0; // 失败次数
    static uint32_t last_tick = 0; // 上次自检时间戳

    if (HAL_GetTick() - last_tick < 100)
        return;
    last_tick = HAL_GetTick();

    // 首次初始化:开机或复位后第一次进入
    if (color_sensor_state == TCS_STA_INIT) {
        if (TCS34725_Init()) {
            color_sensor_state = TCS_STA_RUNNING;
            fail_cnt = 0;
        } else {
            color_sensor_state = TCS_STA_RETRY;
        }
        return;
    }

    // 正常运行:周期读取RGB(非阻塞)
    if (color_sensor_state == TCS_STA_RUNNING) {
        TCS_ReadResult r = TCS_Read_RGB(&color.red, &color.green, &color.blue, &color.clear);

        /* 读取成功 */
        if (r == TCS_READ_OK) {
            color_available = 1;
            fail_cnt = 0;
            return;
        }

        /* 积分未完成,不算故障,保持当前状态 */
        if (r == TCS_READ_NOT_READY) 
            return;

        /* TCS_READ_ERROR: 通信故障 */
        fail_cnt++;
        color_available = 0;

        if (fail_cnt >= 3) {
            color_sensor_state = TCS_STA_RETRY;
        }
        return;
    }

    // 重试:直接重新Init
    if (color_sensor_state == TCS_STA_RETRY) {
        if (TCS34725_Init()) {
            color_sensor_state = TCS_STA_RUNNING;
            fail_cnt = 0;
            return;
        }
        color_sensor_state = TCS_STA_BUS_RESET;
        return;
    }

    // 总线复位:复位I2C后重新Init
    if (color_sensor_state == TCS_STA_BUS_RESET) {
        I2C_Bus_Recover();
        if (TCS34725_Init()) {
            color_sensor_state = TCS_STA_RUNNING;
            fail_cnt = 0;
            return;
        }
        color_sensor_state = TCS_STA_SOFT_RESET;
        return;
    }

    // 软复位:复位传感器后重新Init
    if (color_sensor_state == TCS_STA_SOFT_RESET) {
        TCS_Write(REG_ENABLE, 0x00);
        HAL_Delay(3);
        TCS_Write(REG_ENABLE, PON);
        HAL_Delay(3);

        if (TCS34725_Init()) {
            color_sensor_state = TCS_STA_RUNNING;
            fail_cnt = 0;
            return;
        }

        color_sensor_state = TCS_STA_DEGRADED;
        return;
    }

    // 降级:停止读取,每5秒退回RETRY重试,避免永久死锁
    if (color_sensor_state == TCS_STA_DEGRADED) {
        static uint32_t degraded_tick = 0;

        color_available = 0;

        if (HAL_GetTick() - degraded_tick >= DEGRADED_RETRY_INTERVAL_MS) {
            degraded_tick = HAL_GetTick();
            color_sensor_state = TCS_STA_RETRY;
        }
    }
}

/**
 * @brief 查询颜色传感器数据是否有效
 * @return uint8_t 1有效,0无效
 */
uint8_t IsColorAvailable(void)
{
    return color_available;
}

/**
 * @brief 获取TCS34725传感器的颜色值
 * @return ColorValue 颜色值
 */
ColorValue GetColorValue(void)
{
    return color;
}
