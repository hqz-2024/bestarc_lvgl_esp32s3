#include "demo_data.h"

/**
 * @brief 让值在[min,max]之间循环递增。
 * @usage 每调用一次value+=step，超过max后回到min。
 */
static uint16_t ramp_up(uint16_t value, uint16_t step, uint16_t min_v, uint16_t max_v)
{
    if (max_v <= min_v) {
        return min_v;
    }
    uint32_t v = (uint32_t)value + step;
    if (v > max_v) {
        v = min_v;
    }
    return (uint16_t)v;
}

/**
 * @brief 生成一组演示参数覆盖到state。
 * @usage 每次调用各参数在各自量程内缓慢递增，到顶回到起点。
 */
void demo_data_step(btc500_state_t *state)
{
    static uint16_t tick = 0;

    if (state == NULL) {
        return;
    }
    tick++;

    /* 每30拍切换一次模式/电压/单位，保持显示有变化 */
    if ((tick % 30U) == 0U) {
        state->mode = (btc_mode_t)((state->mode + 1U) % 3U);
    }
    if ((tick % 40U) == 0U) {
        state->tmode = (btc_tmode_t)((state->tmode + 1U) & 0x01U);
    }
    if ((tick % 50U) == 0U) {
        state->input_voltage_v = (state->input_voltage_v == 220U) ? 120U : 220U;
    }
    if ((tick % 60U) == 0U) {
        state->pressure_unit = (btc_pressure_unit_t)((state->pressure_unit + 1U) % 3U);
        state->pressure_value = 0; /* 切单位后从0重新涨 */
    }

    /* 电流：按当前量程线性递增 */
    uint16_t cur_max = btc500_current_max(state);
    state->current_a = ramp_up(state->current_a, 1U, 15U, cur_max);

    /* 气压：按单位量程递增，步进随单位调整避免太慢/太快 */
    uint16_t pr_max = btc500_pressure_max(state);
    uint16_t pr_step = (pr_max >= 100U) ? 2U : ((pr_max >= 10U) ? 1U : 1U);
    state->pressure_value = ramp_up(state->pressure_value, pr_step, 0U, pr_max);

    /* 后气/维弧：3..15 秒循环 */
    state->postflow_s = ramp_up(state->postflow_s, 1U, 3U, 15U);
    state->arcforce_s = ramp_up(state->arcforce_s, 1U, 3U, 15U);

    /* 报警：保持NONE */
    state->alarm = BTC_ALARM_NONE;
}
