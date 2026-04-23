#include "ui_manager.h"
#include <string.h>

#include "screens/scr_dashboard.h"
#include "ui_protocol.h"
#include "demo_data.h"

/* BLE 超时阈值：超过此时间未收到广播则回到演示模式（毫秒） */
#define BLE_TIMEOUT_MS 5000U
/* 演示定时器刷新周期（毫秒） */
#define DEMO_TICK_MS   500U
/* UI刷新（含闪烁）周期（毫秒） */
#define UI_TICK_MS     250U

typedef struct {
    ui_scr_id_t current;
    ui_mode_t   mode;
    scr_dashboard_ctx_t dashboard;
    btc500_state_t state;
    lv_timer_t *tick_timer;
    lv_disp_t  *disp;
    uint32_t    last_ble_ms;
    bool        bt_blink_on;
} ui_manager_ctx_t;

static ui_manager_ctx_t s_ui;

/**
 * @brief 读取大端16位。
 * @usage BLE AD3解析辅助。
 */
static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

/**
 * @brief 刷新仪表盘（含蓝牙闪烁状态）。
 * @usage 由定时器调用，根据当前模式合成state再刷屏。
 */
static void ui_manager_refresh(void)
{
    if (s_ui.current != UI_SCR_DASHBOARD) {
        return;
    }

    /* 蓝牙图标在DEMO模式下闪烁，WORK模式常亮 */
    if (s_ui.mode == UI_MODE_DEMO) {
        s_ui.state.bt_connected = s_ui.bt_blink_on;
    } else {
        s_ui.state.bt_connected = true;
    }

    scr_dashboard_update(&s_ui.dashboard, &s_ui.state);
}

/**
 * @brief 主定时器回调：演示数据生成 / BLE超时检测 / 蓝牙闪烁。
 * @usage 由LVGL定时器每UI_TICK_MS调用。
 */
static void ui_tick_cb(lv_timer_t *t)
{
    (void)t;

    uint32_t now = lv_tick_get();
    s_ui.bt_blink_on = !s_ui.bt_blink_on;

    /* BLE 超时回落到演示模式 */
    if (s_ui.mode == UI_MODE_WORK &&
        (now - s_ui.last_ble_ms) > BLE_TIMEOUT_MS) {
        s_ui.mode = UI_MODE_DEMO;
    }

    /* 演示模式下周期生成随机数据 */
    if (s_ui.mode == UI_MODE_DEMO) {
        static uint32_t last_demo_ms = 0;
        if ((now - last_demo_ms) >= DEMO_TICK_MS) {
            last_demo_ms = now;
            demo_data_step(&s_ui.state);
        }
    }

    ui_manager_refresh();
}

/**
 * @brief 创建并加载仪表盘页面。
 * @usage 页面切换到仪表盘时调用。
 */
static void load_dashboard(void)
{
    lv_coord_t w = lv_disp_get_hor_res(s_ui.disp);
    lv_coord_t h = lv_disp_get_ver_res(s_ui.disp);
    lv_obj_t *scr = scr_dashboard_create(&s_ui.dashboard, w, h);
    lv_scr_load(scr);
    scr_dashboard_update(&s_ui.dashboard, &s_ui.state);
}

/**
 * @brief 初始化UI管理器并加载首页。
 * @usage LVGL初始化完成后调用一次。
 */
void ui_manager_init(void)
{
    memset(&s_ui, 0, sizeof(s_ui));
    s_ui.disp = lv_disp_get_default();
    s_ui.current = UI_SCR_DASHBOARD;
    s_ui.mode = UI_MODE_DEMO;
    s_ui.last_ble_ms = 0;
    s_ui.bt_blink_on = false;

    btc500_state_init(&s_ui.state);
    load_dashboard();

    s_ui.tick_timer = lv_timer_create(ui_tick_cb, UI_TICK_MS, NULL);
}

/**
 * @brief 切换页面。
 * @usage 传入目标页面ID执行切换。
 */
void ui_manager_switch(ui_scr_id_t id)
{
    if (s_ui.current == id) {
        return;
    }
    if (s_ui.current == UI_SCR_DASHBOARD) {
        scr_dashboard_destroy(&s_ui.dashboard);
    }
    s_ui.current = id;
    if (id == UI_SCR_DASHBOARD) {
        load_dashboard();
    }
}

/**
 * @brief 获取当前页面ID。
 * @usage 在业务层查询当前页面状态。
 */
ui_scr_id_t ui_manager_get_current(void)
{
    return s_ui.current;
}

/**
 * @brief 反初始化UI管理器。
 * @usage 退出UI时调用，释放页面和定时器资源。
 */
void ui_manager_deinit(void)
{
    if (s_ui.tick_timer != NULL) {
        lv_timer_del(s_ui.tick_timer);
        s_ui.tick_timer = NULL;
    }
    if (s_ui.current == UI_SCR_DASHBOARD) {
        scr_dashboard_destroy(&s_ui.dashboard);
    }
}

/**
 * @brief 设置UI工作模式。
 * @usage 业务层根据BLE连接状态主动切换。
 */
void ui_manager_set_mode(ui_mode_t mode)
{
    s_ui.mode = mode;
    if (mode == UI_MODE_DEMO) {
        s_ui.state.alarm = BTC_ALARM_NONE;
    }
}

/**
 * @brief 获取当前UI工作模式。
 * @usage 供业务层查询当前模式。
 */
ui_mode_t ui_manager_get_mode(void)
{
    return s_ui.mode;
}

/**
 * @brief 处理BLE收到的AD3 manufacturer数据（14字节有效载荷）。
 * @usage 布局：[设备类型2][钢板2][2T4T2][电流2][后气2][维弧2][单位2]。
 *        注意协议里没有气压值字段，气压随机保留，其他字段实时更新。
 */
void ui_manager_on_ble_state(const uint8_t *payload, size_t len)
{
    /* payload 含义参见 communication protocol.md AD3 第8..21字节：共14字节 */
    if (payload == NULL || len < 14U) {
        return;
    }

    s_ui.state.device_type      = be16(&payload[0]);
    s_ui.state.mode             = (btc_mode_t)(be16(&payload[2]) % 3U);
    s_ui.state.tmode            = (btc_tmode_t)(be16(&payload[4]) & 0x01U);
    s_ui.state.current_a        = be16(&payload[6]);
    s_ui.state.postflow_s       = be16(&payload[8]);
    s_ui.state.arcforce_s       = be16(&payload[10]);
    s_ui.state.pressure_unit    = (btc_pressure_unit_t)(be16(&payload[12]) % 3U);

    /* 气压传感器值由BLE之外的源提供；此处保持/置0 */
    /* 输入电压协议里单独查询，BLE广播未携带，保持默认 */

    s_ui.last_ble_ms = lv_tick_get();
    if (s_ui.mode != UI_MODE_WORK) {
        s_ui.mode = UI_MODE_WORK;
    }
    ui_manager_refresh();
}

/**
 * @brief 输入协议原始包并驱动界面刷新。
 * @usage 串口12字节协议帧可直接灌入此函数（目前未启用）。
 */
bool ui_manager_on_protocol_packet(const uint8_t *packet, size_t len)
{
    bool ok = btc500_parse_packet(packet, len, &s_ui.state);
    if (!ok) {
        return false;
    }
    s_ui.last_ble_ms = lv_tick_get();
    if (s_ui.mode != UI_MODE_WORK) {
        s_ui.mode = UI_MODE_WORK;
    }
    ui_manager_refresh();
    return true;
}
