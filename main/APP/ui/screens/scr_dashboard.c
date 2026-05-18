#include "scr_dashboard.h"
#include <string.h>
#include <math.h>

#include "../ui_cmd.h"
#include "../ui_manager.h"

extern const lv_image_dsc_t left_arc;
extern const lv_image_dsc_t right_arc;
extern const lv_image_dsc_t RING_BG;
extern const lv_image_dsc_t CHIP_BG;
extern const lv_image_dsc_t lighting;

/* 计量条/表盘动画时长 (ms) */
#define ANIM_TIME_MS 400

/* 左右表盘显示方式：1=使用图片素材(从下向上填充) 0=使用LVGL arc组件(单色半弧) */
#define DASHBOARD_USE_ARC_IMAGE 1
#define DASHBOARD_USE_ARC_SCALE 0

/**
 * @brief lighting图标呼吸灯效执行回调。
 * @usage 由lv_anim_t驱动，将opa写入图片对象。
 */
static void lighting_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

/**
 * @brief arc动画执行回调。
 * @usage 由lv_anim_t驱动，把当前中间値写入lv_arc。
 */
static void arc_anim_exec_cb(void *var, int32_t v)
{
    lv_arc_set_value((lv_obj_t *)var, (int16_t)v);
}

/**
 * @brief 以渐变动画方式设置arc的值。
 * @usage 代替lv_arc_set_value，让指针从当前值平滑过渡到目标值。
 */
static void arc_set_value_anim(lv_obj_t *arc, int16_t target)
{
    int16_t cur = lv_arc_get_value(arc);
    if (cur == target) {
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, arc_anim_exec_cb);
    lv_anim_set_values(&a, cur, target);
    lv_anim_set_time(&a, ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

/* 弧图片全高 */
#define ARC_IMG_FULL_H 439


/* ============ 颜色常量（取自 SVG 设计稿） ============ */
#define COLOR_BG          lv_color_hex(0x000000)
#define COLOR_RING        lv_color_hex(0xE40148)
#define COLOR_LEFT_ARC    lv_color_hex(0xFE8C3B)
#define COLOR_RIGHT_ARC   lv_color_hex(0x26D66E)
#define COLOR_VAL         lv_color_hex(0x00F0FF)
#define COLOR_UNIT        lv_color_hex(0x00F0FF)
#define COLOR_CHIP_BG     lv_color_hex(0x4D00FF)
#define COLOR_CHIP_BG_2   lv_color_hex(0x2E0099)
#define COLOR_CHIP_TEXT   lv_color_hex(0xF4F4F4)
#define COLOR_BAR_BG      lv_color_hex(0x1C1616)
#define COLOR_BAR_FG      lv_color_hex(0x05FF00)
#define COLOR_BAR_FG2     lv_color_hex(0x00F0FF)
#define COLOR_BRAND       lv_color_hex(0xE40148)
#define COLOR_ALARM       lv_color_hex(0xE40148)
#define COLOR_BT          lv_color_hex(0x1D97FF)
#define COLOR_BT_DIM      lv_color_hex(0x2E0099)

/* ============ 基准分辨率：800x480（SVG 设计稿） ============ */
#define DSN_W 800
#define DSN_H 480

/**
 * @brief 按设计稿基准缩放X坐标。
 * @usage 输入设计稿像素值返回当前分辨率下的实际坐标。
 */
static inline lv_coord_t sx(lv_coord_t x, lv_coord_t sw)
{
    return (lv_coord_t)((int32_t)x * sw / DSN_W);
}

/**
 * @brief 按设计稿基准缩放Y坐标。
 * @usage 输入设计稿像素值返回当前分辨率下的实际坐标。
 */
static inline lv_coord_t sy(lv_coord_t y, lv_coord_t sh)
{
    return (lv_coord_t)((int32_t)y * sh / DSN_H);
}

/**
 * @brief 创建一个左列chip（紫色圆角矩形+居中文字）。
 * @usage 返回label对象供后续更新文本使用。
 */
static lv_obj_t *create_chip(lv_obj_t *parent,
                              lv_coord_t x, lv_coord_t y,
                              lv_coord_t w, lv_coord_t h,
                              const char *text)
{
    lv_obj_t *chip = lv_image_create(parent);
    lv_image_set_src(chip, &CHIP_BG);
    lv_obj_set_pos(chip, x, y);
    lv_obj_clear_flag(chip, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = lv_label_create(chip);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, COLOR_CHIP_TEXT, 0);
    lv_obj_set_style_text_font(label, &bestarc_BBHBogle_48, 0);
    lv_obj_center(label);
    return label;
}

#if DASHBOARD_USE_ARC_SCALE
/* 刻度尺3段分区数据 */
#define SCALE_SECT_CNT 3
static lv_style_t s_sect_label_sty[2][SCALE_SECT_CNT];
static lv_style_t s_sect_minor_sty[2][SCALE_SECT_CNT];
static lv_style_t s_sect_main_sty[2][SCALE_SECT_CNT];
static const int32_t s_sect_range[2][SCALE_SECT_CNT][2] = {
    {{0,40},{40,80},{80,100}},
    {{0,20},{20,60},{60,100}},
};
static const uint32_t s_sect_color[2][SCALE_SECT_CNT] = {
    {0x05FF00, 0xFD8C3B, 0xFF2A6D},
    {0x4D00FF, 0x00F0FF, 0x05FF00},
};

/**
 * @brief 创建表盘刻度尺（lv_scale，LVGL v9）。
 * @usage side=0左/1右；rotation内部固定120/280，range 0-100。
 */
static lv_obj_t *create_meter_scale(lv_obj_t *parent,
    lv_coord_t x, lv_coord_t y, lv_coord_t size, int side)
{
    lv_obj_t *scale = lv_scale_create(parent);
    lv_obj_set_size(scale, size, size);
    lv_obj_set_pos(scale, x, y);
    lv_obj_update_layout(scale);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_total_tick_count(scale, 41);
    lv_scale_set_major_tick_every(scale, 20);
    lv_scale_set_label_show(scale, false);
    lv_scale_set_range(scale, 0, 100);
    lv_scale_set_angle_range(scale, 140);
    lv_scale_set_rotation(scale, (side == 0) ? 120 : 280);
    lv_scale_set_post_draw(scale, true);
    for (int i = 0; i < SCALE_SECT_CNT; i++) {
        lv_scale_section_t *sec = lv_scale_add_section(scale);
        lv_scale_section_set_range(sec, s_sect_range[side][i][0], s_sect_range[side][i][1]);
        lv_style_init(&s_sect_label_sty[side][i]);
        lv_style_set_line_color(&s_sect_label_sty[side][i], lv_color_hex(s_sect_color[side][i]));
        lv_style_init(&s_sect_minor_sty[side][i]);
        lv_style_set_line_color(&s_sect_minor_sty[side][i], lv_color_hex(s_sect_color[side][i]));
        lv_style_init(&s_sect_main_sty[side][i]);
        lv_style_set_arc_color(&s_sect_main_sty[side][i], lv_color_hex(s_sect_color[side][i]));
        lv_style_set_arc_width(&s_sect_main_sty[side][i], 2);
        lv_scale_section_set_style(sec, LV_PART_INDICATOR, &s_sect_label_sty[side][i]);
        lv_scale_section_set_style(sec, LV_PART_MAIN,      &s_sect_main_sty[side][i]);
        lv_scale_section_set_style(sec, LV_PART_ITEMS,     &s_sect_minor_sty[side][i]);
    }
    lv_obj_set_style_bg_opa(scale,       LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(scale,       220,           LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(scale, 0,             LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(scale, 0,             LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(scale,    2,             LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(scale,      LV_OPA_COVER,  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(scale,    lv_color_hex(0x757575), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(scale,  true,          LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale,       5,             LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale,   2,             LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(scale,   lv_color_hex(0x757575), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(scale,     LV_OPA_COVER,  LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale,       10,            LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale,   2,             LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(scale,   lv_color_hex(0x757575), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(scale,     LV_OPA_COVER,  LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_clear_flag(scale, LV_OBJ_FLAG_CLICKABLE);
    return scale;
}
#endif /* DASHBOARD_USE_ARC_SCALE */

/**
 * @brief 创建右列的垂直进度条（维弧/后气）。
 * @usage 返回bar对象；title/value label通过out参数输出。
 */
static lv_obj_t *create_vbar(lv_obj_t *parent,
                              lv_coord_t x, lv_coord_t y,
                              lv_coord_t w, lv_coord_t h,
                              const char *title,
                              lv_obj_t **val_label_out)
{
    lv_obj_t *bar = lv_slider_create(parent);
    lv_obj_set_size(bar, w, h);
    lv_obj_align(bar, LV_ALIGN_LEFT_MID, x, y);
    lv_slider_set_range(bar, 3, 15);
    lv_slider_set_value(bar, 3, LV_ANIM_OFF);
    lv_obj_set_style_anim_time(bar, ANIM_TIME_MS, 0);
    lv_obj_set_style_bg_color(bar, COLOR_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, w / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, COLOR_BAR_FG2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(bar, COLOR_BAR_FG, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, w / 2, LV_PART_INDICATOR);
    /* 隐藏 slider 的 knob */
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_KNOB);
    /* 让条从顶往下填充 */
    lv_bar_set_mode(bar, LV_BAR_MODE_NORMAL);

    /* 数值标签（覆盖在条下方） */
    lv_obj_t *val = lv_label_create(parent);
    lv_label_set_text(val, "5S");
    lv_obj_set_style_text_color(val, COLOR_CHIP_TEXT, 0);
    lv_obj_set_style_text_font(val, &bestarc_BBHBogle_32, 0);
    lv_obj_align_to(val, bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
    if (val_label_out) {
        *val_label_out = val;
    }

    /* 标题（PILOT arc / POST AIR） */
    lv_obj_t *ttl = lv_label_create(parent);
    lv_label_set_text(ttl, title);
    lv_obj_set_style_text_color(ttl, COLOR_CHIP_TEXT, 0);
    lv_obj_set_style_text_font(ttl, &bestarc_BBHBogle_18, 0);
    lv_obj_align_to(ttl, val, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    return bar;
}

/**
 * @brief 创建一个半圆弧表盘。
 * @usage 返回arc对象，value/unit label通过out参数输出。
 */
static lv_obj_t *create_arc_dial(lv_obj_t *parent,
                                  lv_coord_t cx, lv_coord_t cy, lv_coord_t radius,
                                  uint16_t start_ang, uint16_t end_ang,
                                  lv_color_t arc_color,
                                  const char *init_val, const char *init_unit,
                                  lv_obj_t **val_out, lv_obj_t **unit_out)
{
    lv_coord_t size = radius * 2;
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_remove_style_all(arc);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, cx - radius, cy - radius);
    lv_arc_set_bg_angles(arc, start_ang, end_ang);
    lv_arc_set_rotation(arc, 0);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 18, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, arc_color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    /* 数值大字（居中偏上） */
    lv_obj_t *val = lv_label_create(parent);
    lv_label_set_text(val, init_val);
    lv_obj_set_style_text_color(val, COLOR_VAL, 0);
    lv_obj_set_style_text_font(val, &bestarc_BBHBogle_128, 0);
    lv_obj_align(val, LV_ALIGN_CENTER, cx - lv_obj_get_width(parent)/2, (cy + 75) - lv_obj_get_height(parent)/2);
    if (val_out) {
        *val_out = val;
    }

    /* 单位（在数值下方） */
    lv_obj_t *unit = lv_label_create(parent);
    lv_label_set_text(unit, init_unit);
    lv_obj_set_style_text_color(unit, COLOR_UNIT, 0);
    lv_obj_set_style_text_font(unit, &bestarc_BBHBogle_80, 0);
    lv_obj_align(unit, LV_ALIGN_CENTER, cx - lv_obj_get_width(parent)/2, (cy + 135) - lv_obj_get_height(parent)/2);
    if (unit_out) {
        *unit_out = unit;
    }

    return arc;
}

/**
 * @brief 下发并应用本地命令。
 * @usage 触摸事件回调中统一调用，先更新state再下发。
 */
static void apply_and_send(uint16_t cmd, uint16_t data)
{
    ui_manager_apply_local_cmd(cmd, data);
    ui_cmd_send(cmd, data);
}

/**
 * @brief 左电流弧拖动事件。
 * @usage 按下/拖动期间根据触点角度换算电流并下发。
 */
static void on_left_arc_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    scr_dashboard_ctx_t *ctx = (scr_dashboard_ctx_t *)lv_event_get_user_data(e);
    if (ctx == NULL) return;

    if (code == LV_EVENT_PRESSED) {
        ctx->left_arc_dragging = true;
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ctx->left_arc_dragging = false;
        return;
    } else if (code != LV_EVENT_PRESSING) {
        return;
    }

    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_obj_t *arc = lv_event_get_target(e);
    lv_area_t area;
    lv_obj_get_coords(arc, &area);
    int32_t cx = (area.x1 + area.x2) / 2;
    int32_t cy = (area.y1 + area.y2) / 2;
    int32_t dx = p.x - cx;
    int32_t dy = p.y - cy;
    if (dx == 0 && dy == 0) return;

    float ang = atan2f((float)dy, (float)dx) * 180.0f / 3.14159265f;
    if (ang < 0.0f) ang += 360.0f;

    /* 左弧可视范围 120°(底) ~ 240°(顶) */
    if (ang < 120.0f) ang = 120.0f;
    if (ang > 240.0f) ang = 240.0f;

    btc500_state_t snap;
    ui_manager_get_state(&snap);
    uint16_t cur_max = btc500_current_max(&snap);
    uint16_t cur_min = 15U;
    if (cur_max <= cur_min) return;
    uint16_t span = (uint16_t)(cur_max - cur_min);

    float ratio = (ang - 120.0f) / 120.0f;     /* 顶=1，底=0 */
    uint16_t new_cur = (uint16_t)(cur_min + (uint16_t)(ratio * (float)span + 0.5f));
    if (new_cur < cur_min) new_cur = cur_min;
    if (new_cur > cur_max) new_cur = cur_max;
    if (new_cur != snap.current_a) {
        apply_and_send(0x0400, new_cur);
    }
}

/**
 * @brief PILOT/POST 滑条事件回调。
 * @usage 拖动期间设置标志，VALUE_CHANGED 时下发协议。
 */
static void on_slider_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    scr_dashboard_ctx_t *ctx = (scr_dashboard_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);
    if (ctx == NULL || slider == NULL) return;

    bool is_pilot = (slider == ctx->pilot_bar);
    bool *flag = is_pilot ? &ctx->pilot_dragging : &ctx->postair_dragging;
    uint16_t cmd = is_pilot ? 0x0600 : 0x0500;

    if (code == LV_EVENT_PRESSED) {
        *flag = true;
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        *flag = false;
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        int32_t v = lv_slider_get_value(slider);
        if (v < 3) v = 3;
        if (v > 15) v = 15;
        apply_and_send(cmd, (uint16_t)v);
    }
}

/**
 * @brief 模式 chip 点击循环切换。
 * @usage 钢/网格/除锈三态循环。
 */
static void on_mode_click(lv_event_t *e)
{
    (void)e;
    btc500_state_t snap;
    ui_manager_get_state(&snap);
    uint16_t next = (uint16_t)((snap.mode + 1U) % 3U);
    apply_and_send(0x0200, next);
}

/**
 * @brief 2T/4T chip 点击切换。
 * @usage 0/1 切换。
 */
static void on_tmode_click(lv_event_t *e)
{
    (void)e;
    btc500_state_t snap;
    ui_manager_get_state(&snap);
    uint16_t next = (uint16_t)((snap.tmode + 1U) % 2U);
    apply_and_send(0x0300, next);
}

/**
 * @brief 气压单位标签点击循环。
 * @usage PSI→MPA→BAR 循环。
 */
static void on_unit_click(lv_event_t *e)
{
    (void)e;
    btc500_state_t snap;
    ui_manager_get_state(&snap);
    uint16_t next = (uint16_t)((snap.pressure_unit + 1U) % 3U);
    apply_and_send(0x0700, next);
}

/**
 * @brief 处理仪表盘页面事件。
 * @usage 作为页面统一事件入口。
 */
void scr_dashboard_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_LOADED) {
        lv_obj_clear_flag(lv_event_get_target(e), LV_OBJ_FLAG_SCROLLABLE);
    }
}

/**
 * @brief 创建主仪表盘页面。
 * @usage 传入上下文与分辨率参数，返回创建好的screen对象。
 */
lv_obj_t *scr_dashboard_create(scr_dashboard_ctx_t *ctx, lv_coord_t screen_width, lv_coord_t screen_height)
{
    if (ctx == NULL) {
        return NULL;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->screen_width  = screen_width;
    ctx->screen_height = screen_height;

    const lv_coord_t SW = screen_width;
    const lv_coord_t SH = screen_height;

    /* 背景 */
    ctx->screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(ctx->screen);
    lv_obj_set_size(ctx->screen, SW, SH);
    lv_obj_set_style_bg_opa(ctx->screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(ctx->screen, COLOR_BG, 0);
    lv_obj_set_style_bg_grad_color(ctx->screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_dir(ctx->screen, LV_GRAD_DIR_VER, 0);
    lv_obj_clear_flag(ctx->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_align(ctx->screen, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ctx->screen, scr_dashboard_event_handler, LV_EVENT_ALL, ctx);

    ctx->root = ctx->screen;

    /* 背景图替代外环和品牌标签，放最底层 */
    lv_coord_t ring_cx = sx(400, SW);
    lv_coord_t ring_cy = sy(287, SH);
    lv_obj_t *ring_bg_img = lv_image_create(ctx->root);
    lv_image_set_src(ring_bg_img, &RING_BG);
    lv_obj_set_pos(ring_bg_img, ring_cx - RING_BG.header.w / 2, ring_cy - RING_BG.header.h / 2 - 14);
    lv_obj_clear_flag(ring_bg_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_to_index(ring_bg_img, 0);
    ctx->outer_ring = NULL;

    /* 左表盘（电流）：使用左半圆弧，圆心与外环一致但半径小一些 */
    lv_coord_t dial_r = sx(240, SW);
    ctx->left_arc = create_arc_dial(ctx->root,
        ring_cx, ring_cy, dial_r,
        120, 240,
        COLOR_LEFT_ARC,
        "0", "A",
        &ctx->left_val_label, &ctx->left_unit_label);
    /* 左侧数值位置（设计稿：左数值 x=233 y=162，中心约 290, 240） */
    lv_obj_align(ctx->left_val_label,  LV_ALIGN_CENTER, sx(280, SW) - SW/2, sy(240, SH) - SH/2);
    lv_obj_align(ctx->left_unit_label, LV_ALIGN_CENTER, sx(280, SW) - SW/2, sy(350, SH) - SH/2);

    /* 右表盘（气压）：右半圆弧 */
    ctx->right_arc = create_arc_dial(ctx->root,
        ring_cx, ring_cy, dial_r,
        300, 60,
        COLOR_RIGHT_ARC,
        "0", "MPa",
        &ctx->right_val_label, &ctx->right_unit_label);
    lv_arc_set_mode(ctx->right_arc, LV_ARC_MODE_REVERSE);
    lv_obj_align(ctx->right_val_label,  LV_ALIGN_CENTER, sx(520, SW) - SW/2, sy(240, SH) - SH/2);
    lv_obj_align(ctx->right_unit_label, LV_ALIGN_CENTER, sx(520, SW) - SW/2, sy(350, SH) - SH/2);

#if DASHBOARD_USE_ARC_IMAGE
    /* 用图片素材替代两个LVGL弧控件：先隐藏原弧 */
    lv_obj_add_flag(ctx->left_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctx->right_arc, LV_OBJ_FLAG_HIDDEN);

    /* 图片实际尺寸(来自.c文件header): 203x439，无需缩放 */
    const int32_t ARC_IMG_W = 203;
    const lv_coord_t arc_base_y = ring_cy - ARC_IMG_FULL_H / 2 - 17;

    /* 左弧图片：全量显示，黑色弧裆盖途达第一层 */
    lv_obj_t *left_arc_img = lv_image_create(ctx->root);
    lv_image_set_src(left_arc_img, &left_arc);
    lv_obj_set_pos(left_arc_img, ring_cx - ARC_IMG_W - 40, arc_base_y);
    lv_obj_clear_flag(left_arc_img, LV_OBJ_FLAG_CLICKABLE);
    ctx->left_arc_img = left_arc_img;

    /* 右弧图片：全量显示 */
    lv_obj_t *right_arc_img = lv_image_create(ctx->root);
    lv_image_set_src(right_arc_img, &right_arc);
    lv_obj_set_pos(right_arc_img, ring_cx + 40, arc_base_y);
    lv_obj_clear_flag(right_arc_img, LV_OBJ_FLAG_CLICKABLE);
    ctx->right_arc_img = right_arc_img;

    /* 左黑色裆盖弧： start=120 end=240 REVERSE，初始全覆盖 */
    ctx->left_cover_arc = lv_arc_create(ctx->root);
    lv_obj_remove_style_all(ctx->left_cover_arc);
    lv_obj_set_size(ctx->left_cover_arc, dial_r * 2 + 10, dial_r * 2 + 10);
    lv_obj_set_pos(ctx->left_cover_arc, ring_cx - dial_r - 5, ring_cy - dial_r - 5);
    lv_arc_set_range(ctx->left_cover_arc, 0, 100);
    lv_arc_set_bg_angles(ctx->left_cover_arc, 120, 260);
    lv_arc_set_mode(ctx->left_cover_arc, LV_ARC_MODE_REVERSE);
    lv_arc_set_value(ctx->left_cover_arc, 100);
    lv_obj_set_style_arc_color(ctx->left_cover_arc, COLOR_BG, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx->left_cover_arc, dial_r - 220, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ctx->left_cover_arc, 0, LV_PART_INDICATOR);
    lv_obj_add_flag(ctx->left_cover_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ctx->left_cover_arc, on_left_arc_event, LV_EVENT_PRESSED, ctx);
    lv_obj_add_event_cb(ctx->left_cover_arc, on_left_arc_event, LV_EVENT_PRESSING, ctx);
    lv_obj_add_event_cb(ctx->left_cover_arc, on_left_arc_event, LV_EVENT_RELEASED, ctx);
    lv_obj_add_event_cb(ctx->left_cover_arc, on_left_arc_event, LV_EVENT_PRESS_LOST, ctx);

    /* 右黑色裆盖弧： start=300 end=60 NORMAL，初始全覆盖 */
    ctx->right_cover_arc = lv_arc_create(ctx->root);
    lv_obj_remove_style_all(ctx->right_cover_arc);
    lv_obj_set_size(ctx->right_cover_arc, dial_r * 2 + 10, dial_r * 2 + 10);
    lv_obj_set_pos(ctx->right_cover_arc, ring_cx - dial_r - 5, ring_cy - dial_r - 5);
    lv_arc_set_range(ctx->right_cover_arc, 0, 100);
    lv_arc_set_bg_angles(ctx->right_cover_arc, 278, 60);
    lv_arc_set_mode(ctx->right_cover_arc, LV_ARC_MODE_NORMAL);
    lv_arc_set_value(ctx->right_cover_arc, 100);
    lv_obj_set_style_arc_color(ctx->right_cover_arc, COLOR_BG, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx->right_cover_arc, dial_r - 220, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ctx->right_cover_arc, 0, LV_PART_INDICATOR);
    lv_obj_clear_flag(ctx->right_cover_arc, LV_OBJ_FLAG_CLICKABLE);
#else
    /* 使用LVGL arc组件：保证原弧显示，不创建图片和裆盖弧 */
    ctx->left_arc_img   = NULL;
    ctx->right_arc_img  = NULL;
    ctx->left_cover_arc = NULL;
    ctx->right_cover_arc= NULL;
#endif

#if DASHBOARD_USE_ARC_SCALE
    ctx->left_scale  = create_meter_scale(ctx->root,
        sx(190, SW), sy(77, SH), sx(420, SW), 0);
    ctx->right_scale = create_meter_scale(ctx->root,
        sx(190, SW), sy(77, SH), sx(420, SW), 1);
#endif

    /* 中心闪电图标 */
    ctx->lighting_img = lv_image_create(ctx->root);
    lv_image_set_src(ctx->lighting_img, &lighting);
    lv_obj_set_pos(ctx->lighting_img,
        ring_cx - lighting.header.w / 2,
        ring_cy - lighting.header.h / 2 - 20);
    lv_obj_clear_flag(ctx->lighting_img, LV_OBJ_FLAG_CLICKABLE);
    lv_anim_t la;
    lv_anim_init(&la);
    lv_anim_set_var(&la, ctx->lighting_img);
    lv_anim_set_exec_cb(&la, lighting_opa_cb);
    lv_anim_set_values(&la, 30, 255);
    lv_anim_set_time(&la, 1200);
    lv_anim_set_playback_time(&la, 1200);
    lv_anim_set_repeat_count(&la, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&la, lv_anim_path_ease_in_out);
    lv_anim_start(&la);

    /* 将数值/单位标签移到最前，避免被图片遮住 */
    lv_obj_move_foreground(ctx->left_val_label);
    lv_obj_move_foreground(ctx->left_unit_label);
    lv_obj_move_foreground(ctx->right_val_label);
    lv_obj_move_foreground(ctx->right_unit_label);

    /* 左列三个chip（电压 / 钢材 / 2T4T） 设计稿: x=11 宽127 高71, y=16/204/392 */
    lv_coord_t chip_x = sx(11, SW);
    lv_coord_t chip_w = sx(127, SW);
    lv_coord_t chip_h = sy(71, SH);
    ctx->voltage_chip_label = create_chip(ctx->root, chip_x, sy(16, SH),  chip_w, chip_h, "220V");
    ctx->mode_chip_label    = create_chip(ctx->root, chip_x, sy(204, SH), chip_w, chip_h, "STEEL");
    ctx->tmode_chip_label   = create_chip(ctx->root, chip_x, sy(392, SH), chip_w, chip_h, "2T");

    /* 模式/2T4T chip 启用点击 */
    lv_obj_t *mode_chip  = lv_obj_get_parent(ctx->mode_chip_label);
    lv_obj_t *tmode_chip = lv_obj_get_parent(ctx->tmode_chip_label);
    lv_obj_add_flag(mode_chip,  LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(tmode_chip, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(mode_chip,  on_mode_click,  LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(tmode_chip, on_tmode_click, LV_EVENT_CLICKED, ctx);

    /* 气压单位标签点击循环 */
    lv_obj_add_flag(ctx->right_unit_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(ctx->right_unit_label, 20);
    lv_obj_add_event_cb(ctx->right_unit_label, on_unit_click, LV_EVENT_CLICKED, ctx);

    /* 右侧双垂直条：x/y 为相对屏幕左中的偏移，bar 以左中为锚点 */
    lv_coord_t bar_w = sx(20, SW);
    lv_coord_t bar_h = sy(280, SH);
    lv_coord_t bar_y = 20;  /* 以屏幕垂直中心为基准 */
    ctx->pilot_bar   = create_vbar(ctx->root, sx(670, SW), bar_y, bar_w, bar_h, "PILOT ARC", &ctx->pilot_val_label);
    ctx->postair_bar = create_vbar(ctx->root, sx(745, SW), bar_y, bar_w, bar_h, "POST AIR", &ctx->postair_val_label);

    /* 扩展滑条触摸热区并绑定事件 */
    lv_obj_set_ext_click_area(ctx->pilot_bar,   16);
    lv_obj_set_ext_click_area(ctx->postair_bar, 16);
    lv_obj_add_event_cb(ctx->pilot_bar,   on_slider_event, LV_EVENT_PRESSED,       ctx);
    lv_obj_add_event_cb(ctx->pilot_bar,   on_slider_event, LV_EVENT_RELEASED,      ctx);
    lv_obj_add_event_cb(ctx->pilot_bar,   on_slider_event, LV_EVENT_PRESS_LOST,    ctx);
    lv_obj_add_event_cb(ctx->pilot_bar,   on_slider_event, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(ctx->postair_bar, on_slider_event, LV_EVENT_PRESSED,       ctx);
    lv_obj_add_event_cb(ctx->postair_bar, on_slider_event, LV_EVENT_RELEASED,      ctx);
    lv_obj_add_event_cb(ctx->postair_bar, on_slider_event, LV_EVENT_PRESS_LOST,    ctx);
    lv_obj_add_event_cb(ctx->postair_bar, on_slider_event, LV_EVENT_VALUE_CHANGED, ctx);

    /* 蓝牙图标（右上角） */
    ctx->bt_icon = lv_label_create(ctx->root);
    lv_label_set_text(ctx->bt_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(ctx->bt_icon, COLOR_BT, 0);
    lv_obj_set_style_text_font(ctx->bt_icon, &lv_font_montserrat_48, 0);
    lv_obj_align(ctx->bt_icon, LV_ALIGN_CENTER, sx(760, SW) - SW/2, sy(40, SH) - SH/2);

    /* 顶部报警标签（小字体红色，正上方居中） */
    ctx->alarm_label = lv_label_create(ctx->root);
    lv_label_set_text(ctx->alarm_label, "");
    lv_obj_set_style_text_color(ctx->alarm_label, COLOR_ALARM, 0);
    lv_obj_set_style_text_font(ctx->alarm_label, &bestarc_BBHBogle_18, 0);
    lv_obj_align(ctx->alarm_label, LV_ALIGN_TOP_MID, 0, sy(12, SH));

    ctx->brand_label = NULL;

    return ctx->screen;
}

/**
 * @brief 销毁主仪表盘页面。
 * @usage 页面切换离开时调用，释放页面对象。
 */
void scr_dashboard_destroy(scr_dashboard_ctx_t *ctx)
{
    if (ctx == NULL || ctx->screen == NULL) {
        return;
    }
    lv_obj_del(ctx->screen);
    ctx->screen = NULL;
}

/**
 * @brief 刷新仪表盘显示数据。
 * @usage 协议状态变化后调用以更新界面。
 */
void scr_dashboard_update(scr_dashboard_ctx_t *ctx, const btc500_state_t *state)
{
    char buf[16];

    if (ctx == NULL || state == NULL || ctx->screen == NULL) {
        return;
    }

    /* 电流表盘：量程按模式/电压区分，最小15 */
    if (!ctx->left_arc_dragging) {
        uint16_t cur_max = btc500_current_max(state);
        uint16_t cur_min = 15U;
        uint16_t cur_val = LV_MIN(state->current_a, cur_max);
        if (cur_val < cur_min) cur_val = cur_min;
        uint16_t cur_span = (cur_max > cur_min) ? (uint16_t)(cur_max - cur_min) : 1U;
        arc_set_value_anim(ctx->left_arc, (int16_t)((uint32_t)(cur_val - cur_min) * 100U / cur_span));
#if DASHBOARD_USE_ARC_IMAGE
        arc_set_value_anim(ctx->left_cover_arc,
            100 - (int16_t)((uint32_t)(cur_val - cur_min) * 100U / cur_span));
#endif
        lv_snprintf(buf, sizeof(buf), "%u", state->current_a);
        lv_label_set_text(ctx->left_val_label, buf);
    }

    /* 气压表盘 */
    uint16_t pr_max = btc500_pressure_max(state);
    uint16_t pr_val = LV_MIN(state->pressure_value, pr_max);
    arc_set_value_anim(ctx->right_arc, (int16_t)((uint32_t)pr_val * 100U / (pr_max ? pr_max : 1U)));
#if DASHBOARD_USE_ARC_IMAGE
    arc_set_value_anim(ctx->right_cover_arc,
        100 - (int16_t)((uint32_t)pr_val * 100U / (pr_max ? pr_max : 1U)));
    // arc_set_value_anim(ctx->right_cover_arc,
    //     0);
#endif
    lv_snprintf(buf, sizeof(buf), "%u", state->pressure_value);
    lv_label_set_text(ctx->right_val_label, buf);
    lv_label_set_text(ctx->right_unit_label, btc500_pressure_unit_text(state->pressure_unit));

    /* 左列chip */
    lv_snprintf(buf, sizeof(buf), "%uV", state->input_voltage_v);
    lv_label_set_text(ctx->voltage_chip_label, buf);
    lv_label_set_text(ctx->mode_chip_label, btc500_mode_text(state->mode));
    lv_label_set_text(ctx->tmode_chip_label, btc500_tmode_text(state->tmode));

    /* 右侧双条 */
    if (!ctx->pilot_dragging) {
        lv_slider_set_value(ctx->pilot_bar, state->arcforce_s, LV_ANIM_ON);
        lv_snprintf(buf, sizeof(buf), "%uS", state->arcforce_s);
        lv_label_set_text(ctx->pilot_val_label, buf);
    }

    if (!ctx->postair_dragging) {
        lv_slider_set_value(ctx->postair_bar, state->postflow_s, LV_ANIM_ON);
        lv_snprintf(buf, sizeof(buf), "%uS", state->postflow_s);
        lv_label_set_text(ctx->postair_val_label, buf);
    }

    /* 蓝牙图标颜色 */
    lv_obj_set_style_text_color(ctx->bt_icon,
        state->bt_connected ? COLOR_BT : COLOR_BT_DIM, 0);

    /* 报警文字（红色小字在顶部） */
    if (state->alarm == BTC_ALARM_NONE) {
        lv_label_set_text(ctx->alarm_label, "");
    } else {
        lv_label_set_text(ctx->alarm_label, btc500_alarm_text(state->alarm));
    }
}
