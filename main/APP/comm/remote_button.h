#ifndef APP_REMOTE_BUTTON_H
#define APP_REMOTE_BUTTON_H

#include "esp_err.h"

/* 启动 BOOT 按键监控：5 次连按（窗口约 3s）清 NVS 中 MAC 并重启。 */
esp_err_t remote_button_start(void);

#endif
