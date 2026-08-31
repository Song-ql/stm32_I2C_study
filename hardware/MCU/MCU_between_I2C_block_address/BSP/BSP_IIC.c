#include "BSP_IIC.h"
#include "i2c.h"

/* 阻塞式 I2C 收发, 不使用中断回调 */

/* 粗略微秒级延时 (用于 I2C 总线恢复时序, 不依赖 HAL_Delay/SysTick)
 * 100kHz I2C: SCL 高电平需 >=4.0us, 循环常数经验值, 72MHz 主频下约 5~10us
 */
static void BSP_IIC_DelayUs(void)
{
    uint32_t i;
    for (i = 0; i < 200; i++)
    {
        __NOP();
    }
}

/* I2C 外设恢复: DeInit + 最多 9 个 SCL 脉冲清总线 + STOP + ReInit
 * 用于 STM32F1 I2C 外设锁死 (BUSY 标志位卡住) 时恢复总线
 * 流程:
 *   1. HAL_I2C_DeInit 反初始化外设 (HAL_I2C_MspDeInit 把 GPIO 复位为默认输入)
 *   2. 临时把 SCL/SDA 切换为开漏输出, 发送最多 9 个 SCL 脉冲,
 *      期间 SDA 一旦被释放为高即提前结束 (清除从机锁死的 ACK 状态)
 *   3. 产生一个 STOP 条件释放总线
 *   4. 调用 reinit (如 MX_I2C1_Init) 重新初始化外设 (HAL_I2C_MspInit 重配 GPIO 为 AF_OD)
 */
void BSP_IIC_Recover(I2C_HandleTypeDef *hi2c,
                     GPIO_TypeDef *port, uint16_t scl_pin, uint16_t sda_pin,
                     void (*reinit)(void))
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    int i;

    /* 1. 反初始化 I2C 外设 */
    HAL_I2C_DeInit(hi2c);

    /* 2. 临时把 SCL/SDA 配置为开漏输出, 用于手动产生 SCL 脉冲 */
    GPIO_InitStruct.Pin = scl_pin | sda_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    /* SCL/SDA 初始拉高 */
    HAL_GPIO_WritePin(port, scl_pin | sda_pin, GPIO_PIN_SET);

    /* 发送最多 9 个 SCL 脉冲, SDA 一旦被释放为高即提前结束 */
    for (i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
        BSP_IIC_DelayUs();
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
        BSP_IIC_DelayUs();
        if (HAL_GPIO_ReadPin(port, sda_pin) == GPIO_PIN_SET)
        {
            break;
        }
    }

    /* 3. 产生一个 STOP 条件: SCL 高时 SDA 由低变高 */
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_RESET);
    BSP_IIC_DelayUs();
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_SET);
    BSP_IIC_DelayUs();

    /* 4. 重新初始化 I2C 外设 (HAL_I2C_MspInit 会把 GPIO 重新配置为 AF_OD) */
    if (reinit != NULL)
    {
        reinit();
    }
}
