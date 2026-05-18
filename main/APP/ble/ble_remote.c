#include "ble_remote.h"
#include "device_config.h"
#include "sdkconfig.h"
#include "esp_log.h"

#if (DEVICE_ROLE == DEVICE_ROLE_REMOTE) && defined(CONFIG_BT_NIMBLE_ENABLED)

#include <string.h>
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "remote_nvs.h"
#include "ui_manager.h"

#define SVC_UUID16     0xFFE0
#define CHR_UUID16     0xFFE1
#define CFG_DEV_NAME   "BYS_remote"
#define TGT_DEV_NAME   "BYS"
#define PKT_LEN        12
#define CFG_MAGIC_LO   0xFF   /* byte6 */
#define CFG_MAGIC_HI   0x00   /* byte7 */

static const char *TAG = "ble_remote";

typedef enum { MODE_CONFIG, MODE_NORMAL } remote_mode_t;

static remote_mode_t s_mode;
static uint8_t  s_target_mac[6];
static uint8_t  s_own_addr_type;
static uint16_t s_chr_val_handle;
static uint16_t s_conn_handle    = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_peer_chr_handle;
static bool     s_subscribed;
static esp_timer_handle_t s_restart_tmr;

static int normal_gap_event(struct ble_gap_event *e, void *arg);
static int cfg_gap_event(struct ble_gap_event *e, void *arg);
static void normal_scan_start(void);
static void cfg_adv_start(void);

/* ========== 配置模式：BYS_remote 外设 ========== */

/* 收到 App 写入：校验魔数后保存 MAC，1s 后重启。 */
static int cfg_chr_access(uint16_t conn, uint16_t attr,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn; (void)attr; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return 0;
    }
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len != PKT_LEN) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    uint8_t buf[PKT_LEN]; uint16_t out = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &out) != 0 || out != PKT_LEN) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (buf[6] != CFG_MAGIC_LO || buf[7] != CFG_MAGIC_HI) {
        ESP_LOGW(TAG, "cfg magic mismatch");
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (remote_nvs_set_mac(buf) != ESP_OK) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    /* 异步 1s 后重启 */
    if (s_restart_tmr) {
        esp_timer_start_once(s_restart_tmr, 1000 * 1000);
    }
    return 0;
}

static const struct ble_gatt_chr_def s_cfg_chr_defs[] = {
    {
        .uuid = BLE_UUID16_DECLARE(CHR_UUID16),
        .access_cb = cfg_chr_access,
        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP |
                 BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &s_chr_val_handle,
    },
    { 0 },
};

static const struct ble_gatt_svc_def s_cfg_svc_defs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(SVC_UUID16),
        .characteristics = s_cfg_chr_defs,
    },
    { 0 },
};

/* 启动配置模式广播。 */
static void cfg_adv_start(void)
{
    if (ble_gap_adv_active()) {
        return;
    }
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.name = (uint8_t *)CFG_DEV_NAME;
    fields.name_len = strlen(CFG_DEV_NAME);
    fields.name_is_complete = 1;
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    (void)ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params p = {0};
    p.conn_mode = BLE_GAP_CONN_MODE_UND;
    p.disc_mode = BLE_GAP_DISC_MODE_GEN;
    p.itvl_min = 0x00A0;
    p.itvl_max = 0x00A0;
    int rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                               &p, cfg_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "cfg adv rc=%d", rc);
    }
}

static int cfg_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status != 0) {
            cfg_adv_start();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        cfg_adv_start();
        break;
    default:
        break;
    }
    return 0;
}

/* ========== 正常模式：Central 扫描连接主设备 ========== */

/* 广播匹配：Local Name == "BYS" 且 Manuf Data 前 6 字节 == NVS MAC。 */
static bool match_target(const struct ble_gap_disc_desc *d)
{
    struct ble_hs_adv_fields f;
    if (ble_hs_adv_parse_fields(&f, d->data, d->length_data) != 0) {
        return false;
    }
    if (f.name == NULL || f.name_len != strlen(TGT_DEV_NAME)) {
        return false;
    }
    if (memcmp(f.name, TGT_DEV_NAME, f.name_len) != 0) {
        return false;
    }
    if (f.mfg_data == NULL || f.mfg_data_len < 6) {
        return false;
    }
    return memcmp(f.mfg_data, s_target_mac, 6) == 0;
}

static void normal_scan_start(void)
{
    if (ble_gap_disc_active()) {
        return;
    }
    struct ble_gap_disc_params p = {0};
    p.itvl = 0x0050;
    p.window = 0x0030;
    p.passive = 0;
    p.filter_duplicates = 1;
    int rc = ble_gap_disc(s_own_addr_type, BLE_HS_FOREVER, &p, normal_gap_event, NULL);
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        ESP_LOGE(TAG, "disc rc=%d", rc);
    }
}

/* GATT 字符发现回调：找到 0xFFE1 后订阅 Notify。 */
static int disc_chrs_cb(uint16_t conn, const struct ble_gatt_error *err,
                        const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    if (chr != NULL && err != NULL && err->status == 0 &&
        ble_uuid_u16(&chr->uuid.u) == CHR_UUID16) {
        s_peer_chr_handle = chr->val_handle;
    }
    if (err != NULL && err->status == BLE_HS_EDONE) {
        if (s_peer_chr_handle != 0 && !s_subscribed) {
            uint8_t v[2] = {0x01, 0x00};
            /* CCCD 紧跟值句柄之后，简单实现：val_handle + 1 */
            (void)ble_gattc_write_no_rsp_flat(conn, s_peer_chr_handle + 1, v, sizeof(v));
            s_subscribed = true;
            ESP_LOGI(TAG, "subscribed chr=0x%04X", s_peer_chr_handle);
        }
    }
    return 0;
}

static int normal_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        if (match_target(&event->disc)) {
            ble_gap_disc_cancel();
            int rc = ble_gap_connect(s_own_addr_type, &event->disc.addr,
                                     5000, NULL, normal_gap_event, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "connect rc=%d", rc);
                normal_scan_start();
            }
        }
        break;
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn_handle = event->connect.conn_handle;
            s_peer_chr_handle = 0;
            s_subscribed = false;
            ui_manager_set_app_connected(true);
            (void)ble_gattc_disc_all_chrs(s_conn_handle, 1, 0xFFFF, disc_chrs_cb, NULL);
            ESP_LOGI(TAG, "connected handle=%d", s_conn_handle);
        } else {
            normal_scan_start();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        s_peer_chr_handle = 0;
        s_subscribed = false;
        ui_manager_set_app_connected(false);
        ESP_LOGI(TAG, "disconnected reason=%d", event->disconnect.reason);
        normal_scan_start();
        break;
    case BLE_GAP_EVENT_NOTIFY_RX:
        if (event->notify_rx.attr_handle == s_peer_chr_handle) {
            uint16_t len = OS_MBUF_PKTLEN(event->notify_rx.om);
            if (len == PKT_LEN) {
                uint8_t buf[PKT_LEN]; uint16_t got = 0;
                if (ble_hs_mbuf_to_flat(event->notify_rx.om, buf, sizeof(buf), &got) == 0) {
                    ui_manager_on_protocol_packet(buf, PKT_LEN);
                }
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

/* ========== 公共：同步、宿主任务、入口 ========== */

static void on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "addr infer rc=%d", rc);
        return;
    }
    if (s_mode == MODE_CONFIG) {
        cfg_adv_start();
        ESP_LOGI(TAG, "config-mode advertising as %s", CFG_DEV_NAME);
    } else {
        normal_scan_start();
        ESP_LOGI(TAG, "normal-mode scanning for %s", TGT_DEV_NAME);
    }
}

static void host_task(void *p)
{
    (void)p;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void restart_timer_cb(void *arg)
{
    (void)arg;
    esp_restart();
}

esp_err_t remote_ble_start(void)
{
    if (remote_nvs_init() != ESP_OK) {
        return ESP_FAIL;
    }
    bool have_mac = remote_nvs_get_mac(s_target_mac);
#if REMOTE_DEBUG_FIXED_MAC
    s_target_mac[0] = REMOTE_DEBUG_FIXED_MAC_B0;
    s_target_mac[1] = REMOTE_DEBUG_FIXED_MAC_B1;
    s_target_mac[2] = REMOTE_DEBUG_FIXED_MAC_B2;
    s_target_mac[3] = REMOTE_DEBUG_FIXED_MAC_B3;
    s_target_mac[4] = REMOTE_DEBUG_FIXED_MAC_B4;
    s_target_mac[5] = REMOTE_DEBUG_FIXED_MAC_B5;
    have_mac = true;
    ESP_LOGW(TAG, "DEBUG fixed mac %02X:%02X:%02X:%02X:%02X:%02X",
             s_target_mac[0], s_target_mac[1], s_target_mac[2],
             s_target_mac[3], s_target_mac[4], s_target_mac[5]);
#endif
    s_mode = have_mac ? MODE_NORMAL : MODE_CONFIG;

    const esp_timer_create_args_t ta = {
        .callback = restart_timer_cb, .name = "rmt_rst",
    };
    (void)esp_timer_create(&ta, &s_restart_tmr);

    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble init %d", err);
        return err;
    }

    ble_hs_cfg.sync_cb = on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();

    if (s_mode == MODE_CONFIG) {
        int rc = ble_gatts_count_cfg(s_cfg_svc_defs);
        if (rc == 0) rc = ble_gatts_add_svcs(s_cfg_svc_defs);
        if (rc != 0) {
            ESP_LOGE(TAG, "cfg gatts rc=%d", rc);
            return ESP_FAIL;
        }
        (void)ble_svc_gap_device_name_set(CFG_DEV_NAME);
    } else {
        (void)ble_svc_gap_device_name_set(CFG_DEV_NAME);
    }

    nimble_port_freertos_init(host_task);
    return ESP_OK;
}

bool remote_ble_is_config_mode(void)
{
    return s_mode == MODE_CONFIG;
}

esp_err_t remote_ble_write_pkt(const uint8_t *pkt, size_t len)
{
    if (s_mode != MODE_NORMAL) return ESP_ERR_INVALID_STATE;
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_peer_chr_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (pkt == NULL || len != PKT_LEN) return ESP_ERR_INVALID_ARG;
    int rc = ble_gattc_write_no_rsp_flat(s_conn_handle, s_peer_chr_handle, pkt, len);
    return (rc == 0) ? ESP_OK : ESP_FAIL;
}

#else /* DEVICE_ROLE != REMOTE 或未启用 NimBLE */

esp_err_t remote_ble_start(void) { return ESP_ERR_NOT_SUPPORTED; }
bool remote_ble_is_config_mode(void) { return false; }
esp_err_t remote_ble_write_pkt(const uint8_t *pkt, size_t len)
{ (void)pkt; (void)len; return ESP_ERR_NOT_SUPPORTED; }

#endif
