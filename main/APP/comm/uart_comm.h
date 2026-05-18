#ifndef APP_UART_COMM_H
#define APP_UART_COMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define UART_COMM_PKT_LEN   12U

/**
 * @brief 初始化UART1（TX:IO12 RX:IO13 19200 8N1）并启动轮询任务。
 * @usage 在通讯任务入口调用一次；任务跑在调用者所在Core。
 */
esp_err_t uart_comm_start(void);

/**
 * @brief 将APP下发的12字节原始包入队转发到下位机。
 * @usage BLE FFE1 写回调里直接传入APP写入的12字节。
 */
esp_err_t uart_comm_send_app_cmd(const uint8_t *pkt, size_t len);

/**
 * @brief 设置APP连接状态，用于查询包device_type字段填充。
 * @usage BLE 连接/断开事件中调用。
 */
void uart_comm_set_app_connected(bool connected);

#endif
