#ifndef APP_BLE_SERVER_H
#define APP_BLE_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief 初始化NimBLE并启动GATT外设（服务FFE0/特征FFE1）+ 厂商广播。
 * @usage 通讯任务初始化阶段调用一次；未启用NimBLE时返回ESP_ERR_NOT_SUPPORTED。
 */
esp_err_t ble_server_start(void);

/**
 * @brief 触发广播数据更新（按当前state重新填充AD3字段）。
 * @usage 设备状态变化后由ui_manager调用；可在任意任务上下文调用。
 */
void ble_server_notify_state_changed(void);

/**
 * @brief 主动向已连接APP推送一帧12字节数据（FFE1 Notify）。
 * @usage 下位机响应或状态变化时上报，未连接时静默丢弃。
 */
void ble_server_notify_packet(const uint8_t *pkt, size_t len);

#endif
