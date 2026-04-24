#ifndef UI_SCREENS_SCR_DASHBOARD_H
#define UI_SCREENS_SCR_DASHBOARD_H

#include "lvgl.h"
#include "../ui_protocol.h"

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *root;
    /* 顶部报警+蓝牙 */
    lv_obj_t *alarm_label;
    lv_obj_t *bt_icon;
    /* 左列三chip：电压 / 钢材 / 2T4T */
    lv_obj_t *voltage_chip_label;
    lv_obj_t *mode_chip_label;
    lv_obj_t *tmode_chip_label;
    /* 中央外环 */
    lv_obj_t *outer_ring;
    /* 左表盘：电流 */
    lv_obj_t *left_scale;
    lv_obj_t *left_arc;
    lv_obj_t *left_arc_img;
    lv_obj_t *left_cover_arc;
    lv_obj_t *left_val_label;
    lv_obj_t *left_unit_label;
    /* 右表盘：气压 */
    lv_obj_t *right_scale;
    lv_obj_t *right_arc;
    lv_obj_t *right_arc_img;
    lv_obj_t *right_cover_arc;
    lv_obj_t *right_val_label;
    lv_obj_t *right_unit_label;
    /* 右侧双垂直条：维弧 / 后气 */
    lv_obj_t *pilot_bar;
    lv_obj_t *pilot_val_label;
    lv_obj_t *postair_bar;
    lv_obj_t *postair_val_label;
    /* 中心图标 */
    lv_obj_t *lighting_img;
    /* 底部品牌 */
    lv_obj_t *brand_label;
    /* 尺寸 */
    lv_coord_t screen_width;
    lv_coord_t screen_height;
} scr_dashboard_ctx_t;

/**
 * @brief 创建主仪表盘页面。
 * @usage 传入上下文与分辨率参数，返回创建好的screen对象。
 */
lv_obj_t *scr_dashboard_create(scr_dashboard_ctx_t *ctx, lv_coord_t screen_width, lv_coord_t screen_height);

/**
 * @brief 销毁主仪表盘页面。
 * @usage 页面切换离开时调用，释放页面对象。
 */
void scr_dashboard_destroy(scr_dashboard_ctx_t *ctx);

/**
 * @brief 处理仪表盘页面事件。
 * @usage 事件回调内部调用，做轻量交互处理。
 */
void scr_dashboard_event_handler(lv_event_t *e);

/**
 * @brief 刷新仪表盘显示数据。
 * @usage 协议状态变化后调用以更新界面。
 */
void scr_dashboard_update(scr_dashboard_ctx_t *ctx, const btc500_state_t *state);

#endif
