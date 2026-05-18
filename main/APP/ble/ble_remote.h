#ifndef APP_BLE_REMOTE_H
#define APP_BLE_REMOTE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* 启动遥控器端 BLE：读 NVS 决定配置模式（外设 BYS_remote）或正常模式（Central 扫连 BYS）。 */
esp_err_t remote_ble_start(void);

/* 是否处于配置模式（无绑定 MAC，等待 App 写入）。 */
bool remote_ble_is_config_mode(void);

/* 正常模式下向主设备透传 12 字节协议包（Write Without Response）。 */
esp_err_t remote_ble_write_pkt(const uint8_t *pkt, size_t len);

#endif
