#include "scr_dashboard.h"
#include <string.h>

extern const lv_image_dsc_t left_arc;
extern const lv_image_dsc_t right_arc;
extern const lv_image_dsc_t RING_BG;
extern const lv_image_dsc_t CHIP_BG;

/* 计量条/表盘动画时长 (ms) */
#define ANIM_TIME_MS 400

/* 左右表盘显示方式：1=使用图片素材(从下向上填充) 0=使用LVGL arc组件(单色半弧) */
#define DASHBOARD_USE_ARC_IMAGE 1

/**
 * @brief arc动画执行回调。
 * @usage 由lv_anim_t驱动，把当前中间值写入lv_arc。
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
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, w, h);
    lv_obj_align(bar, LV_ALIGN_LEFT_MID, x, y);
    lv_bar_set_range(bar, 3, 15);
    lv_bar_set_value(bar, 3, LV_ANIM_OFF);
    lv_obj_set_style_anim_time(bar, ANIM_TIME_MS, 0);
    lv_obj_set_style_bg_color(bar, COLOR_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, w / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, COLOR_BAR_FG2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(bar, COLOR_BAR_FG, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, w / 2, LV_PART_INDICATOR);
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
    lv_obj_align(ctx->left_val_label,  LV_ALIGN_CENTER, sx(300, SW) - SW/2, sy(225, SH) - SH/2);
    lv_obj_align(ctx->left_unit_label, LV_ALIGN_CENTER, sx(300, SW) - SW/2, sy(350, SH) - SH/2);

    /* 右表盘（气压）：右半圆弧 */
    ctx->right_arc = create_arc_dial(ctx->root,
        ring_cx, ring_cy, dial_r,
        300, 60,
        COLOR_RIGHT_ARC,
        "0", "MPa",
        &ctx->right_val_label, &ctx->right_unit_label);
    lv_arc_set_mode(ctx->right_arc, LV_ARC_MODE_REVERSE);
    lv_obj_align(ctx->right_val_label,  LV_ALIGN_CENTER, sx(500, SW) - SW/2, sy(225, SH) - SH/2);
    lv_obj_align(ctx->right_unit_label, LV_ALIGN_CENTER, sx(500, SW) - SW/2, sy(350, SH) - SH/2);

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
    lv_obj_clear_flag(ctx->left_cover_arc, LV_OBJ_FLAG_CLICKABLE);

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

    /* 右侧双垂直条：x/y 为相对屏幕左中的偏移，bar 以左中为锚点 */
    lv_coord_t bar_w = sx(20, SW);
    lv_coord_t bar_h = sy(280, SH);
    lv_coord_t bar_y = 20;  /* 以屏幕垂直中心为基准 */
    ctx->pilot_bar   = create_vbar(ctx->root, sx(670, SW), bar_y, bar_w, bar_h, "PILOT ARC", &ctx->pilot_val_label);
    ctx->postair_bar = create_vbar(ctx->root, sx(745, SW), bar_y, bar_w, bar_h, "POST AIR", &ctx->postair_val_label);

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

    /* 电流表盘：量程15-30A映射到0-100 */
    uint16_t cur_max = btc500_current_max(state);
    uint16_t cur_min = 15U;
    uint16_t cur_val = LV_MIN(state->current_a, cur_max);
    if (cur_val < cur_min) cur_val = cur_min;
    uint16_t cur_span = (cur_max > cur_min) ? (uint16_t)(cur_max - cur_min) : 1U;
    arc_set_value_anim(ctx->left_arc, (int16_t)((uint32_t)(cur_val - cur_min) * 100U / cur_span));
#if DASHBOARD_USE_ARC_IMAGE
    arc_set_value_anim(ctx->left_cover_arc,
        100 - (int16_t)((uint32_t)(cur_val - cur_min) * 100U / cur_span));
    // arc_set_value_anim(ctx->left_cover_arc,
    //     0);
#endif
    lv_snprintf(buf, sizeof(buf), "%u", state->current_a);
    lv_label_set_text(ctx->left_val_label, buf);

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
    lv_bar_set_value(ctx->pilot_bar, state->arcforce_s, LV_ANIM_ON);
    lv_snprintf(buf, sizeof(buf), "%uS", state->arcforce_s);
    lv_label_set_text(ctx->pilot_val_label, buf);

    lv_bar_set_value(ctx->postair_bar, state->postflow_s, LV_ANIM_ON);
    lv_snprintf(buf, sizeof(buf), "%uS", state->postflow_s);
    lv_label_set_text(ctx->postair_val_label, buf);

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
