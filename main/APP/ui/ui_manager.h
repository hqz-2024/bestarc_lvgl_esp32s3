#ifndef UI_UI_MANAGER_H
#define UI_UI_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"
#include "ui_protocol.h"

typedef enum {
    UI_SCR_DASHBOARD = 0,
    UI_SCR_MAX
} ui_scr_id_t;

typedef enum {
    UI_MODE_DEMO = 0,   /* 演示模式：随机参数 */
    UI_MODE_WORK = 1    /* 工作模式：蓝牙实际数据 */
} ui_mode_t;

/**
 * @brief 设置UI工作模式。
 * @usage 业务层根据BLE连接状态主动切换。
 */
void ui_manager_set_mode(ui_mode_t mode);

/**
 * @brief 获取当前UI工作模式。
 * @usage 供业务层查询当前模式。
 */
ui_mode_t ui_manager_get_mode(void);

/**
 * @brief BLE广播数据回调（由ble_scan调用）。
 * @usage 解析出的BTC500状态传入后会刷新界面并自动切换到WORK模式。
 */
void ui_manager_on_ble_state(const uint8_t *ad3_payload, size_t len);

/**
 * @brief 初始化UI管理器并加载首页。
 * @usage LVGL初始化完成后调用一次。
 */
void ui_manager_init(void);

/**
 * @brief 切换页面。
 * @usage 传入目标页面ID执行切换。
 */
void ui_manager_switch(ui_scr_id_t id);

/**
 * @brief 获取当前页面ID。
 * @usage 在业务层查询当前页面状态。
 */
ui_scr_id_t ui_manager_get_current(void);

/**
 * @brief 反初始化UI管理器。
 * @usage 退出UI时调用，释放页面和定时器资源。
 */
void ui_manager_deinit(void);

/**
 * @brief 输入协议原始包并驱动界面刷新。
 * @usage 接收到12字节协议数据后调用，返回true表示解析成功并已刷新界面。
 *        线程安全：内部使用mutex保护state，并自动lvgl_port_lock刷新UI。
 */
bool ui_manager_on_protocol_packet(const uint8_t *packet, size_t len);

/**
 * @brief 线程安全获取当前state副本。
 * @usage BLE任务构建广播包/通知时调用。
 */
void ui_manager_get_state(btc500_state_t *out);

/**
 * @brief 设置APP（BLE）连接状态。
 * @usage BLE连接/断开事件中调用，用于UI蓝牙图标点亮控制。
 */
void ui_manager_set_app_connected(bool connected);

/**
 * @brief 本地触摸操作即时更新state并刷新页面。
 * @usage 仅在LVGL事件回调内调用，配合ui_cmd_send向下位机/主设备下发。
 */
void ui_manager_apply_local_cmd(uint16_t cmd, uint16_t data);

#endif
