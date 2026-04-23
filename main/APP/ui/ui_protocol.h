#ifndef UI_UI_PROTOCOL_H
#define UI_UI_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BTC_MODE_STEEL = 0,
    BTC_MODE_GRID = 1,
    BTC_MODE_RUST = 2
} btc_mode_t;

typedef enum {
    BTC_TMODE_2T = 0,
    BTC_TMODE_4T = 1
} btc_tmode_t;

typedef enum {
    BTC_PRESSURE_PSI = 0,
    BTC_PRESSURE_MPA = 1,
    BTC_PRESSURE_BAR = 2
} btc_pressure_unit_t;

typedef enum {
    BTC_ALARM_NONE = 0,
    BTC_ALARM_OVERCURRENT = 1,
    BTC_ALARM_OVERHEAT = 2
} btc_alarm_t;

typedef struct {
    uint16_t device_type;
    btc_mode_t mode;
    btc_tmode_t tmode;
    uint16_t current_a;
    uint16_t postflow_s;
    uint16_t arcforce_s;
    btc_pressure_unit_t pressure_unit;
    btc_alarm_t alarm;
    uint16_t input_voltage_v;
    uint16_t pressure_value;
    bool bt_connected;
} btc500_state_t;

/**
 * @brief 获取当前模式下的电流上限。
 * @usage 用于电流表盘量程换算。
 */
uint16_t btc500_current_max(const btc500_state_t *state);

/**
 * @brief 获取气压量程上限（根据单位）。
 * @usage 用于气压表盘量程换算。
 */
uint16_t btc500_pressure_max(const btc500_state_t *state);

/**
 * @brief 初始化协议状态默认值。
 * @usage 在系统启动时调用一次。
 */
void btc500_state_init(btc500_state_t *state);

/**
 * @brief 解析12字节协议包并更新状态。
 * @usage 输入完整协议帧，返回true表示解析成功。
 */
bool btc500_parse_packet(const uint8_t *packet, size_t len, btc500_state_t *state);

/**
 * @brief 使用协议字段生成一组模拟状态。
 * @usage 在无串口数据阶段周期调用以驱动界面演示。
 */
void btc500_mock_update(btc500_state_t *state);

/**
 * @brief 获取模式文本。
 * @usage 用于UI显示模式字符串。
 */
const char *btc500_mode_text(btc_mode_t mode);

/**
 * @brief 获取2T/4T文本。
 * @usage 用于UI显示焊接模式字符串。
 */
const char *btc500_tmode_text(btc_tmode_t tmode);

/**
 * @brief 获取气压单位文本。
 * @usage 用于UI显示压力单位。
 */
const char *btc500_pressure_unit_text(btc_pressure_unit_t unit);

/**
 * @brief 获取报警文本。
 * @usage 用于UI显示报警状态。
 */
const char *btc500_alarm_text(btc_alarm_t alarm);

#endif
