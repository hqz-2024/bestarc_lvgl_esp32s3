#include "ble_scan.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "ui_manager.h"

#ifdef CONFIG_BT_NIMBLE_ENABLED

#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "host/ble_gap.h"
#include <string.h>

static const char *TAG = "ble_scan";

/**
 * @brief 在广播数据中查找指定AD type的第一段payload。
 * @usage 成功返回非NULL指针并输出长度；未找到返回NULL。
 */
static const uint8_t *find_ad(const uint8_t *data, uint8_t total_len,
                               uint8_t ad_type, uint8_t *payload_len_out)
{
    uint8_t i = 0;
    while (i + 2 <= total_len) {
        uint8_t len = data[i];
        if (len == 0 || i + 1 + len > total_len) {
            return NULL;
        }
        uint8_t type = data[i + 1];
        if (type == ad_type) {
            if (payload_len_out) {
                *payload_len_out = (uint8_t)(len - 1);
            }
            return &data[i + 2];
        }
        i += (uint8_t)(len + 1);
    }
    return NULL;
}

/**
 * @brief GAP事件回调：接收到广播时解析并上送BTC500状态。
 * @usage NimBLE栈内部调用。
 */
static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    if (event->type != BLE_GAP_EVENT_DISC) {
        return 0;
    }

    const uint8_t *data = event->disc.data;
    uint8_t total = event->disc.length_data;
    uint8_t name_len = 0;
    uint8_t mfg_len = 0;

    const uint8_t *name = find_ad(data, total, 0x09, &name_len); /* Complete Local Name */
    if (name == NULL || name_len < 3U ||
        memcmp(name, "BYS", 3) != 0) {
        return 0;
    }

    const uint8_t *mfg = find_ad(data, total, 0xFF, &mfg_len); /* Manufacturer Specific */
    if (mfg == NULL || mfg_len < 20U) {
        /* AD3 自定义部分总长=6(MAC)+14(字段)=20字节 */
        return 0;
    }

    /* 跳过6字节MAC，剩下14字节为业务字段 */
    ui_manager_on_ble_state(mfg + 6, (size_t)(mfg_len - 6));
    return 0;
}

/**
 * @brief NimBLE栈同步完成后启动主动扫描。
 * @usage 作为ble_hs_cfg.sync_cb在栈就绪时自动调用。
 */
static void on_sync(void)
{
    uint8_t addr_type = 0;
    int rc = ble_hs_id_infer_auto(0, &addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "addr infer fail %d", rc);
        return;
    }

    struct ble_gap_disc_params p = {
        .itvl = 0x30,
        .window = 0x20,
        .filter_policy = 0,
        .limited = 0,
        .passive = 1,
        .filter_duplicates = 0,
    };
    rc = ble_gap_disc(addr_type, BLE_HS_FOREVER, &p, gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_disc fail %d", rc);
    } else {
        ESP_LOGI(TAG, "scanning started");
    }
}

/**
 * @brief NimBLE宿主任务入口。
 * @usage 由nimble_port_freertos_init启动。
 */
static void host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/**
 * @brief 初始化并启动BLE观察者模式扫描。
 * @usage 在主任务初始化阶段调用一次；扫描到BYS设备的厂商数据后会调用ui_manager_on_ble_state()。
 */
esp_err_t ble_scan_start(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble init fail: %d", err);
        return err;
    }

    ble_hs_cfg.sync_cb = on_sync;
    nimble_port_freertos_init(host_task);
    return ESP_OK;
}

/**
 * @brief 停止BLE扫描并释放资源。
 * @usage 退出蓝牙通讯场景时调用。
 */
void ble_scan_stop(void)
{
    ble_gap_disc_cancel();
    nimble_port_stop();
}

#else /* !CONFIG_BT_NIMBLE_ENABLED */

/**
 * @brief BLE未启用时的占位启动函数。
 * @usage 需要在menuconfig中打开BT_ENABLED + BT_NIMBLE_ENABLED才会生效。
 */
esp_err_t ble_scan_start(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

/**
 * @brief BLE未启用时的占位停止函数。
 * @usage 与ble_scan_start配对的空实现。
 */
void ble_scan_stop(void) {}

#endif
