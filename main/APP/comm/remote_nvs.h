#ifndef APP_REMOTE_NVS_H
#define APP_REMOTE_NVS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* 初始化遥控器 NVS 命名空间（bys_remote）。 */
esp_err_t remote_nvs_init(void);

/* 读取已绑定主设备 MAC，命中返回 true。 */
bool remote_nvs_get_mac(uint8_t mac[6]);

/* 写入主设备 MAC 到 NVS。 */
esp_err_t remote_nvs_set_mac(const uint8_t mac[6]);

/* 清除主设备 MAC。 */
esp_err_t remote_nvs_clear_mac(void);

#endif
