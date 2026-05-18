#include "ui_cmd.h"
#include "device_config.h"

#include <stddef.h>

#if (DEVICE_ROLE == DEVICE_ROLE_MASTER)
#include "uart_comm.h"
#else
#include "ble_remote.h"
#endif

#define PKT_LEN  12

/* 小端写入 16 位。 */
static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

esp_err_t ui_cmd_send(uint16_t cmd, uint16_t data)
{
    uint8_t pkt[PKT_LEN];
    wr16(&pkt[0], 0x55AAu);
    wr16(&pkt[2], 0x8000u);
    wr16(&pkt[4], cmd);
    wr16(&pkt[6], data);
    wr16(&pkt[8], (uint16_t)(cmd + data));
    wr16(&pkt[10], 0x55BBu);
#if (DEVICE_ROLE == DEVICE_ROLE_MASTER)
    return uart_comm_send_app_cmd(pkt, PKT_LEN);
#else
    return remote_ble_write_pkt(pkt, PKT_LEN);
#endif
}
