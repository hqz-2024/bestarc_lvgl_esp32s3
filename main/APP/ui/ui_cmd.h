#ifndef UI_UI_CMD_H
#define UI_UI_CMD_H

#include <stdint.h>
#include "esp_err.h"

/* 构造 12 字节协议包并按角色下发（主→UART，遥控→BLE 写主设备）。 */
esp_err_t ui_cmd_send(uint16_t cmd, uint16_t data);

#endif
