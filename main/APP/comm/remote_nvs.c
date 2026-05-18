#include "remote_nvs.h"
#include "device_config.h"

#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define NS_NAME      "bys_remote"
#define KEY_MAC      "mac"

static const char *TAG = "remote_nvs";

/* 初始化 NVS（与 BLE 共用底层 flash 分区，重复 nvs_flash_init 安全）。 */
esp_err_t remote_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

bool remote_nvs_get_mac(uint8_t mac[6])
{
    if (mac == NULL) return false;
    nvs_handle_t h;
    if (nvs_open(NS_NAME, NVS_READONLY, &h) != ESP_OK) return false;
    size_t len = 6;
    esp_err_t err = nvs_get_blob(h, KEY_MAC, mac, &len);
    nvs_close(h);
    return (err == ESP_OK && len == 6);
}

esp_err_t remote_nvs_set_mac(const uint8_t mac[6])
{
    if (mac == NULL) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS_NAME, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, KEY_MAC, mac, 6);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "saved mac %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    return err;
}

esp_err_t remote_nvs_clear_mac(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS_NAME, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_erase_key(h, KEY_MAC);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) {
        (void)nvs_commit(h);
        err = ESP_OK;
    }
    nvs_close(h);
    return err;
}
