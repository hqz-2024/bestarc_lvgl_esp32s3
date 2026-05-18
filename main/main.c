#include "board.h"
#include "device_config.h"
#include "ui_manager.h"
#include "uart_comm.h"
#include "ble_server.h"
#include "ble_remote.h"
#include "remote_nvs.h"
#include "remote_button.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Core 分配：0=UI/LVGL，1=通讯（BLE+串口） */
#define UI_CORE   0
#define COMM_CORE 1

#if (DEVICE_ROLE == DEVICE_ROLE_REMOTE)
/* 创建配置模式 UI：全屏提示文字 + MAC 副标题。 */
static void build_config_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "请使用 app 进行初始化配置");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -20);

    uint8_t mac[6] = {0};
    char buf[48] = {0};
    esp_read_mac(mac, ESP_MAC_BT);
    snprintf(buf, sizeof(buf), "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    lv_obj_t *sub = lv_label_create(scr);
    lv_label_set_text(sub, buf);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x1D97FF), 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 30);

    lv_scr_load(scr);
}
#endif

/* 通讯任务：在 Core 1 启动通讯模块（主/遥控器二选一） */
static void comm_task(void *arg)
{
    (void)arg;
#if (DEVICE_ROLE == DEVICE_ROLE_MASTER)
    uart_comm_start();
    ble_server_start();
#else
    remote_button_start();
    remote_ble_start();
#endif
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_touch_init());
    ESP_ERROR_CHECK(app_lvgl_init());   /* LVGL task 已固定 Core 0 */

#if (DEVICE_ROLE == DEVICE_ROLE_REMOTE)
    /* 提前初始化 NVS 以判定是否处于配置模式 */
    (void)remote_nvs_init();
    uint8_t mac_tmp[6];
    bool have_mac = remote_nvs_get_mac(mac_tmp);
#if REMOTE_DEBUG_FIXED_MAC
    have_mac = true;
#endif

    lvgl_port_lock(0);
    if (have_mac) {
        ui_manager_init();
    } else {
        build_config_screen();
    }
    lvgl_port_unlock();
#else
    lvgl_port_lock(0);
    ui_manager_init();
    lvgl_port_unlock();
#endif

    xTaskCreatePinnedToCore(comm_task, "comm", 4096, NULL, 5, NULL, COMM_CORE);
}
