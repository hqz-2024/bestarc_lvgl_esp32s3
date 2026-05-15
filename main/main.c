#include "board.h"
#include "ui_manager.h"
#include "uart_comm.h"
#include "ble_server.h"
#include "freertos/task.h"

/* Core 分配：0=UI/LVGL，1=通讯（BLE+串口） */
#define UI_CORE   0
#define COMM_CORE 1

/* 通讯任务：在 Core 1 启动 UART1 轮询 + BLE GATT 外设 */
static void comm_task(void *arg)
{
    (void)arg;
    uart_comm_start();
    ble_server_start();
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_touch_init());
    ESP_ERROR_CHECK(app_lvgl_init());   /* LVGL task 已固定 Core 0 */

    lvgl_port_lock(0);
    ui_manager_init();
    lvgl_port_unlock();

    xTaskCreatePinnedToCore(comm_task, "comm", 4096, NULL, 5, NULL, COMM_CORE);
}
