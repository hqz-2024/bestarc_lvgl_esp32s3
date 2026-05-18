#include "ble_server.h"
#include "device_config.h"
#include "sdkconfig.h"
#include "esp_log.h"

#if (DEVICE_ROLE == DEVICE_ROLE_MASTER) && defined(CONFIG_BT_NIMBLE_ENABLED)

#include <string.h>
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "ui_manager.h"
#include "ui_protocol.h"
#include "uart_comm.h"

#define DEVICE_NAME      "BYS"
#define SVC_UUID16       0xFFE0
#define CHR_UUID16       0xFFE1
#define ADV_FIELD_LEN    31
#define FFE1_PKT_LEN     12
#define MAX_LINK         2

static const char *TAG = "ble_server";

static uint16_t s_conn_handles[MAX_LINK] = { BLE_HS_CONN_HANDLE_NONE, BLE_HS_CONN_HANDLE_NONE };
static bool     s_notify_en[MAX_LINK];
static uint8_t  s_conn_mask;
static uint16_t s_char_val_handle;
static uint8_t  s_own_addr_type;
static uint8_t  s_adv_buf[ADV_FIELD_LEN];

/* 分配连接槽，失败返回 -1。 */
static int slot_alloc(uint16_t h)
{
    for (int i = 0; i < MAX_LINK; i++) {
        if (s_conn_handles[i] == BLE_HS_CONN_HANDLE_NONE) {
            s_conn_handles[i] = h;
            s_conn_mask |= (uint8_t)(1u << i);
            return i;
        }
    }
    return -1;
}

/* 释放连接槽。 */
static void slot_free(uint16_t h)
{
    for (int i = 0; i < MAX_LINK; i++) {
        if (s_conn_handles[i] == h) {
            s_conn_handles[i] = BLE_HS_CONN_HANDLE_NONE;
            s_notify_en[i] = false;
            s_conn_mask &= (uint8_t)~(1u << i);
            return;
        }
    }
}

/* 通过 handle 查找槽位 idx，未找到返回 -1。 */
static int slot_find(uint16_t h)
{
    for (int i = 0; i < MAX_LINK; i++) {
        if (s_conn_handles[i] == h) return i;
    }
    return -1;
}

/* AD3 字段偏移（adv_buf 内，MAC 占 10..15） */
#define O_AD3_DEVTYPE   16
#define O_AD3_MODE      18
#define O_AD3_T2T4      20
#define O_AD3_CURRENT   22
#define O_AD3_POSTGAS   24
#define O_AD3_ARC       26
#define O_AD3_UNIT      28
#define O_AD3_ALARM     30

/**
 * @brief 用当前state填充AD3字段并提交。
 * @usage 状态刷新或连接事件后调用。
 */
static void adv_apply_state(void)
{
    btc500_state_t st;
    ui_manager_get_state(&st);
    put_le16(&s_adv_buf[O_AD3_DEVTYPE], st.device_type);
    put_le16(&s_adv_buf[O_AD3_MODE],    (uint16_t)st.mode);
    put_le16(&s_adv_buf[O_AD3_T2T4],    (uint16_t)st.tmode);
    put_le16(&s_adv_buf[O_AD3_CURRENT], st.current_a);
    put_le16(&s_adv_buf[O_AD3_POSTGAS], st.postflow_s);
    put_le16(&s_adv_buf[O_AD3_ARC],     st.arcforce_s);
    put_le16(&s_adv_buf[O_AD3_UNIT],    (uint16_t)st.pressure_unit);
    s_adv_buf[O_AD3_ALARM] = (uint8_t)st.alarm;
    (void)ble_gap_adv_set_data(s_adv_buf, ADV_FIELD_LEN);
}

/**
 * @brief 初始化广播缓冲固定字段并填入本机MAC。
 * @usage on_sync 时调用一次。
 */
static void adv_buf_init(void)
{
    memset(s_adv_buf, 0, sizeof(s_adv_buf));
    /* AD1 Flags */
    s_adv_buf[0] = 0x02; s_adv_buf[1] = 0x01; s_adv_buf[2] = 0x06;
    /* AD2 Complete Local Name "BYS" */
    s_adv_buf[3] = 0x04; s_adv_buf[4] = 0x09;
    s_adv_buf[5] = 'B';  s_adv_buf[6] = 'Y'; s_adv_buf[7] = 'S';
    /* AD3 Manufacturer Specific (length=0x16=22, type=0xFF, payload=21B) */
    s_adv_buf[8] = 0x16; s_adv_buf[9] = 0xFF;
    /* MAC（大端，便于显示）占 [10..15] */
    uint8_t addr[6] = {0};
    if (ble_hs_id_copy_addr(s_own_addr_type, addr, NULL) == 0) {
        for (int i = 0; i < 6; i++) {
            s_adv_buf[10 + i] = addr[5 - i];
        }
    }
}

/* 前置声明，供 adv_start 使用 */
static int gap_event_cb(struct ble_gap_event *event, void *arg);

/**
 * @brief 启动广播。
 * @usage on_sync 或断连后调用。
 */
static void adv_start(void)
{
    if (ble_gap_adv_active()) {
        return;
    }
    struct ble_gap_adv_params p = {0};
    p.conn_mode = BLE_GAP_CONN_MODE_UND;
    p.disc_mode = BLE_GAP_DISC_MODE_GEN;
    p.itvl_min = 0x00A0;  /* 100ms */
    p.itvl_max = 0x00A0;
    int rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                               &p, gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start rc=%d", rc);
    }
}

/**
 * @brief FFE1 读写访问回调。
 * @usage NimBLE 内部通过 ble_gatt_access_fn 调用。
 */
static int chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle; (void)attr_handle; (void)arg;
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len != FFE1_PKT_LEN) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        uint8_t buf[FFE1_PKT_LEN];
        uint16_t out_len = 0;
        if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &out_len) != 0 ||
            out_len != FFE1_PKT_LEN) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        ESP_LOGI(TAG, "APP RX %02X%02X cmd=%02X%02X", buf[0], buf[1], buf[4], buf[5]);
        uart_comm_send_app_cmd(buf, FFE1_PKT_LEN);
        return 0;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        /* 返回上一次notify的内容占位（这里返回空，依据协议APP通常订阅Notify） */
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

/* GATT 服务表 */
static const struct ble_gatt_chr_def s_chr_defs[] = {
    {
        .uuid = BLE_UUID16_DECLARE(CHR_UUID16),
        .access_cb = chr_access_cb,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                 BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &s_char_val_handle,
    },
    { 0 },
};

static const struct ble_gatt_svc_def s_svc_defs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(SVC_UUID16),
        .characteristics = s_chr_defs,
    },
    { 0 },
};

/**
 * @brief GAP事件回调：连接/断开/订阅。
 * @usage NimBLE 内部调用。
 */
static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            uint16_t h = event->connect.conn_handle;
            int idx = slot_alloc(h);
            ESP_LOGI(TAG, "connected handle=%d slot=%d mask=0x%02X", h, idx, s_conn_mask);
            uart_comm_set_app_connected(s_conn_mask != 0);
            ui_manager_set_app_connected(s_conn_mask != 0);
            /* 未占满时继续广播，接纳第二条连接 */
            if (s_conn_mask != ((1u << MAX_LINK) - 1u)) {
                adv_start();
            }
        } else {
            adv_start();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        slot_free(event->disconnect.conn.conn_handle);
        ESP_LOGI(TAG, "disconnected reason=%d mask=0x%02X",
                 event->disconnect.reason, s_conn_mask);
        uart_comm_set_app_connected(s_conn_mask != 0);
        ui_manager_set_app_connected(s_conn_mask != 0);
        adv_start();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == s_char_val_handle) {
            int idx = slot_find(event->subscribe.conn_handle);
            if (idx >= 0) {
                s_notify_en[idx] = event->subscribe.cur_notify;
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

/**
 * @brief NimBLE 同步完成回调：注册服务并启动广播。
 * @usage ble_hs_cfg.sync_cb。
 */
static void on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "addr infer rc=%d", rc);
        return;
    }
    adv_buf_init();
    adv_apply_state();
    adv_start();
    ESP_LOGI(TAG, "advertising started");
}

/**
 * @brief NimBLE 宿主任务入口。
 */
static void host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t ble_server_start(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;

    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble init %d", err);
        return err;
    }

    ble_hs_cfg.sync_cb = on_sync;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    int rc = ble_gatts_count_cfg(s_svc_defs);
    if (rc == 0) rc = ble_gatts_add_svcs(s_svc_defs);
    if (rc != 0) {
        ESP_LOGE(TAG, "gatts add svcs rc=%d", rc);
        return ESP_FAIL;
    }
    (void)ble_svc_gap_device_name_set(DEVICE_NAME);

    nimble_port_freertos_init(host_task);
    return ESP_OK;
}

void ble_server_notify_state_changed(void)
{
    if (ble_hs_synced()) {
        adv_apply_state();
    }
}

void ble_server_notify_packet(const uint8_t *pkt, size_t len)
{
    if (pkt == NULL || len != FFE1_PKT_LEN || s_conn_mask == 0) {
        return;
    }
    for (int i = 0; i < MAX_LINK; i++) {
        if (s_conn_handles[i] == BLE_HS_CONN_HANDLE_NONE || !s_notify_en[i]) {
            continue;
        }
        struct os_mbuf *om = ble_hs_mbuf_from_flat(pkt, (uint16_t)len);
        if (om == NULL) continue;
        (void)ble_gatts_notify_custom(s_conn_handles[i], s_char_val_handle, om);
    }
}

#else /* DEVICE_ROLE != MASTER 或未启用 NimBLE */

esp_err_t ble_server_start(void) { return ESP_ERR_NOT_SUPPORTED; }
void ble_server_notify_state_changed(void) {}
void ble_server_notify_packet(const uint8_t *pkt, size_t len) { (void)pkt; (void)len; }

#endif
