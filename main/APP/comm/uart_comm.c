#include "uart_comm.h"

#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "ui_manager.h"

#define UART_PORT          UART_NUM_1
#define UART_TX_PIN        GPIO_NUM_12
#define UART_RX_PIN        GPIO_NUM_13
#define UART_BAUD          19200
#define UART_RX_BUF        256
#define UART_TX_BUF        256
#define UART_TX_QUEUE_LEN  8
#define UART_POLL_MS       100
#define UART_INTER_PKT_MS  20

#define DEV_APP_ON         0x8000U
#define DEV_APP_OFF        0x0000U

static const char *TAG = "uart_comm";

static const uint16_t s_query_cmds[] = {
    0x0002, 0x0003, 0x0004, 0x0005,
    0x0006, 0x0007, 0x0008, 0x0009,
};
#define QUERY_COUNT (sizeof(s_query_cmds) / sizeof(s_query_cmds[0]))

static QueueHandle_t s_tx_queue;
static volatile bool s_app_connected;
static uint8_t s_rx_acc[UART_COMM_PKT_LEN * 2];
static size_t s_rx_acc_len;

/**
 * @brief 小端写入16位整数。
 * @usage 协议字段填充。
 */
static inline void w16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}

/**
 * @brief 构造12字节协议包。
 * @usage 内部用于轮询查询。
 */
static void build_pkt(uint8_t *pkt, uint16_t dev_type, uint16_t cmd, uint16_t data)
{
    uint16_t chksum = (uint16_t)(cmd + data);
    w16le(&pkt[0], 0x55AA);
    w16le(&pkt[2], dev_type);
    w16le(&pkt[4], cmd);
    w16le(&pkt[6], data);
    w16le(&pkt[8], chksum);
    w16le(&pkt[10], 0x55BB);
}

/**
 * @brief 立即写串口一包并等待短暂间隔。
 * @usage 任务内部串行发送。
 */
static void uart_send_pkt(const uint8_t *pkt)
{
    uart_write_bytes(UART_PORT, (const char *)pkt, UART_COMM_PKT_LEN);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(50));
}

/**
 * @brief 累积RX字节并按头尾切包，解析后送UI。
 * @usage 收到新数据后调用。
 */
static void process_rx(const uint8_t *buf, size_t len)
{
    if (s_rx_acc_len + len > sizeof(s_rx_acc)) {
        s_rx_acc_len = 0;
    }
    memcpy(&s_rx_acc[s_rx_acc_len], buf, len);
    s_rx_acc_len += len;

    size_t i = 0;
    while (s_rx_acc_len - i >= UART_COMM_PKT_LEN) {
        if (s_rx_acc[i] == 0xAA && s_rx_acc[i + 1] == 0x55 &&
            s_rx_acc[i + 10] == 0xBB && s_rx_acc[i + 11] == 0x55) {
            ui_manager_on_protocol_packet(&s_rx_acc[i], UART_COMM_PKT_LEN);
            i += UART_COMM_PKT_LEN;
        } else {
            i++;
        }
    }
    if (i > 0) {
        s_rx_acc_len -= i;
        if (s_rx_acc_len > 0) {
            memmove(s_rx_acc, &s_rx_acc[i], s_rx_acc_len);
        }
    }
}

/**
 * @brief UART通讯任务：轮询查询 + RX解析 + APP透传出队。
 * @usage uart_comm_start内部创建。
 */
static void uart_task(void *arg)
{
    (void)arg;
    uint8_t rx_buf[64];
    uint8_t tx_pkt[UART_COMM_PKT_LEN];
    uint32_t last_poll = 0;
    uint32_t query_idx = 0;

    while (1) {
        int n = uart_read_bytes(UART_PORT, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(10));
        if (n > 0) {
            process_rx(rx_buf, (size_t)n);
        }

        if (xQueueReceive(s_tx_queue, tx_pkt, 0) == pdTRUE) {
            uart_send_pkt(tx_pkt);
            vTaskDelay(pdMS_TO_TICKS(UART_INTER_PKT_MS));
            continue;
        }

        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((now - last_poll) >= UART_POLL_MS) {
            last_poll = now;
            uint16_t dev = s_app_connected ? DEV_APP_ON : DEV_APP_OFF;
            build_pkt(tx_pkt, dev, s_query_cmds[query_idx], 0x0000);
            query_idx = (query_idx + 1) % QUERY_COUNT;
            uart_send_pkt(tx_pkt);
        }
    }
}

esp_err_t uart_comm_start(void)
{
    s_tx_queue = xQueueCreate(UART_TX_QUEUE_LEN, UART_COMM_PKT_LEN);
    if (s_tx_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    uart_config_t cfg = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_RX_BUF, UART_TX_BUF, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    BaseType_t ok = xTaskCreate(uart_task, "uart_comm", 4096, NULL, 6, NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "UART1 started tx=%d rx=%d baud=%d", UART_TX_PIN, UART_RX_PIN, UART_BAUD);
    return ESP_OK;
}

esp_err_t uart_comm_send_app_cmd(const uint8_t *pkt, size_t len)
{
    if (pkt == NULL || len != UART_COMM_PKT_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t buf[UART_COMM_PKT_LEN];
    memcpy(buf, pkt, UART_COMM_PKT_LEN);
    /* 强制device_type=APP连接（小端：低字节在前） */
    buf[2] = 0x00;
    buf[3] = 0x80;
    if (xQueueSend(s_tx_queue, buf, 0) != pdTRUE) {
        ESP_LOGW(TAG, "tx queue full");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void uart_comm_set_app_connected(bool connected)
{
    s_app_connected = connected;
}
