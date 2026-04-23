#include "ui_protocol.h"

#include <string.h>

#define BTC_HEAD 0x55AAu
#define BTC_TAIL 0x55BBu

/**
 * @brief 读取大端16位数据。
 * @usage 传入2字节起始地址，返回uint16_t值。
 */
static uint16_t read_u16_be(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | data[1];
}

/**
 * @brief 限幅16位数值。
 * @usage 用于协议值裁剪到指定范围。
 */
static uint16_t clamp_u16(uint16_t value, uint16_t min_v, uint16_t max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

/**
 * @brief 根据协议响应码更新状态字段。
 * @usage 由解析函数内部调用。
 */
static void apply_report_cmd(btc500_state_t *state, uint16_t cmd, uint16_t data)
{
    switch (cmd) {
    case 0x0082:
        state->mode = (btc_mode_t)clamp_u16(data, 0, 2);
        break;
    case 0x0083:
        state->tmode = (btc_tmode_t)clamp_u16(data, 0, 1);
        break;
    case 0x0084:
        state->current_a = clamp_u16(data, 15, 50);
        break;
    case 0x0085:
        state->postflow_s = clamp_u16(data, 3, 15);
        break;
    case 0x0086:
        state->arcforce_s = clamp_u16(data, 3, 15);
        break;
    case 0x0087:
        state->pressure_unit = (btc_pressure_unit_t)clamp_u16(data, 0, 2);
        break;
    case 0x0088:
        state->alarm = (btc_alarm_t)clamp_u16(data, 0, 2);
        break;
    case 0x0089:
        state->input_voltage_v = (data == 0) ? 120 : 240;
        break;
    case 0x8100:
        state->alarm = BTC_ALARM_OVERCURRENT;
        break;
    default:
        break;
    }
}

/**
 * @brief 获取当前模式下的电流上限。
 * @usage 用于电流表盘量程换算。
 */
uint16_t btc500_current_max(const btc500_state_t *state)
{
    (void)state;
    return 30U;
}

/**
 * @brief 获取气压量程上限（根据单位）。
 * @usage 用于气压表盘量程换算。
 */
uint16_t btc500_pressure_max(const btc500_state_t *state)
{
    (void)state;
    return 20U;
}

/**
 * @brief 初始化协议状态默认值。
 * @usage 在系统启动时调用一次。
 */
void btc500_state_init(btc500_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->device_type = 0x0002;
    state->mode = BTC_MODE_STEEL;
    state->tmode = BTC_TMODE_2T;
    state->current_a = 30;
    state->postflow_s = 6;
    state->arcforce_s = 8;
    state->pressure_unit = BTC_PRESSURE_MPA;
    state->alarm = BTC_ALARM_NONE;
    state->input_voltage_v = 220;
    state->pressure_value = 0;
    state->bt_connected = false;
}

/**
 * @brief 解析12字节协议包并更新状态。
 * @usage 输入完整协议帧，返回true表示解析成功。
 */
bool btc500_parse_packet(const uint8_t *packet, size_t len, btc500_state_t *state)
{
    uint16_t head;
    uint16_t device_type;
    uint16_t cmd;
    uint16_t data;
    uint16_t checksum;
    uint16_t tail;

    if (packet == NULL || state == NULL || len != 12U) {
        return false;
    }

    head = read_u16_be(&packet[0]);
    device_type = read_u16_be(&packet[2]);
    cmd = read_u16_be(&packet[4]);
    data = read_u16_be(&packet[6]);
    checksum = read_u16_be(&packet[8]);
    tail = read_u16_be(&packet[10]);

    if (head != BTC_HEAD || tail != BTC_TAIL) {
        return false;
    }

    if ((uint16_t)(cmd + data) != checksum) {
        return false;
    }

    state->device_type = device_type;
    apply_report_cmd(state, cmd, data);
    return true;
}

/**
 * @brief 使用协议字段生成一组模拟状态。
 * @usage 在无串口数据阶段周期调用以驱动界面演示。
 */
void btc500_mock_update(btc500_state_t *state)
{
    static uint16_t step = 0;

    if (state == NULL) {
        return;
    }

    step = (uint16_t)((step + 1U) % 60U);

    state->current_a = (uint16_t)(20U + (step % 30U));
    state->pressure_value = state->current_a;
    state->input_voltage_v = (step % 2U == 0U) ? 220U : 240U;
    state->postflow_s = (uint16_t)(3U + (step % 13U));
    state->arcforce_s = (uint16_t)(3U + ((step + 4U) % 13U));
    state->mode = (btc_mode_t)(step % 3U);
    state->tmode = (btc_tmode_t)(step % 2U);
    state->pressure_unit = (btc_pressure_unit_t)(step % 3U);
    state->alarm = (step % 15U == 0U) ? BTC_ALARM_OVERHEAT : BTC_ALARM_NONE;
}

/**
 * @brief 获取模式文本。
 * @usage 用于UI显示模式字符串。
 */
const char *btc500_mode_text(btc_mode_t mode)
{
    switch (mode) {
    case BTC_MODE_GRID:
        return "GRID";
    case BTC_MODE_RUST:
        return "RUST";
    case BTC_MODE_STEEL:
    default:
        return "STEEL";
    }
}

/**
 * @brief 获取2T/4T文本。
 * @usage 用于UI显示焊接模式字符串。
 */
const char *btc500_tmode_text(btc_tmode_t tmode)
{
    return (tmode == BTC_TMODE_4T) ? "4T" : "2T";
}

/**
 * @brief 获取气压单位文本。
 * @usage 用于UI显示压力单位。
 */
const char *btc500_pressure_unit_text(btc_pressure_unit_t unit)
{
    switch (unit) {
    case BTC_PRESSURE_MPA:
        return "MPA";
    case BTC_PRESSURE_BAR:
        return "BAR";
    case BTC_PRESSURE_PSI:
    default:
        return "PSI";
    }
}

/**
 * @brief 获取报警文本。
 * @usage 用于UI显示报警状态。
 */
const char *btc500_alarm_text(btc_alarm_t alarm)
{
    switch (alarm) {
    case BTC_ALARM_OVERCURRENT:
        return "ALARM: OVERCURRENT";
    case BTC_ALARM_OVERHEAT:
        return "ALARM: OVERHEAT";
    case BTC_ALARM_NONE:
    default:
        return "ALARM: NONE";
    }
}
