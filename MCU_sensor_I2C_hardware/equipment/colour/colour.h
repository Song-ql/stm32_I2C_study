#ifndef __COLOUR_H_
#define __COLOUR_H_

#include "i2c.h"

/**
 * @brief 颜色值结构体
 */
typedef struct {
    uint16_t red;   // 红色值
    uint16_t green; // 绿色值
    uint16_t blue;  // 蓝色值
    uint16_t clear; // 环境光总强度
} ColorValue;

/**
 * @brief 状态机更新,内部完成读取与自愈
 * @note  由主循环周期性调用
 */
void TCS34725_Run(void);

/**
 * @brief 查询颜色传感器数据是否有效
 * @return uint8_t 1有效,0无效
 */
uint8_t IsColorAvailable(void);

/**
 * @brief 获取TCS34725传感器的颜色值
 * @return ColorValue 颜色值
 */
ColorValue GetColorValue(void);

#endif
