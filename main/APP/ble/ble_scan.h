#ifndef APP_BLE_SCAN_H
#define APP_BLE_SCAN_H

#include "esp_err.h"

/**
 * @brief 初始化并启动BLE观察者模式扫描。
 * @usage 在主任务初始化阶段调用一次；扫描到BYS设备的厂商数据后会调用ui_manager_on_ble_state()。
 */
esp_err_t ble_scan_start(void);

/**
 * @brief 停止BLE扫描并释放资源。
 * @usage 退出蓝牙通讯场景时调用。
 */
void ble_scan_stop(void);

#endif
