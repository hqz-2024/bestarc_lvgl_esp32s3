#include "remote_button.h"
#include "remote_nvs.h"
#include "board.h"
#include "iot_button.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "remote_btn";

/* 5 连按回调：清 MAC 并重启进入配置模式。 */
static void on_multi_click(void *handle, void *usr_data)
{
    (void)handle; (void)usr_data;
    ESP_LOGW(TAG, "BOOT x5: clearing mac and restarting");
    (void)remote_nvs_clear_mac();
    esp_restart();
}

esp_err_t remote_button_start(void)
{
    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 0,
        .short_press_time = 0,
        .gpio_button_config = {
            .gpio_num = BOOT_BUTTON_NUM,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    button_handle_t btn = iot_button_create(&cfg);
    if (btn == NULL) {
        ESP_LOGE(TAG, "create fail");
        return ESP_FAIL;
    }
    button_event_config_t ec = {
        .event = BUTTON_MULTIPLE_CLICK,
        .event_data.multiple_clicks.clicks = 5,
    };
    return iot_button_register_event_cb(btn, ec, on_multi_click, NULL);
}
