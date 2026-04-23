#ifndef UI_DEMO_DATA_H
#define UI_DEMO_DATA_H

#include "ui_protocol.h"

/**
 * @brief 生成一组演示参数覆盖到state。
 * @usage 演示模式定时器回调中调用，每次输出不同范围内的随机值。
 */
void demo_data_step(btc500_state_t *state);

#endif
