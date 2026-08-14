/**
 * @file    ws_client.c
 * @brief   WebSocket 客户端实现 — 连接云端服务器
 *
 * 照合作者 01_led 的 ws_client.c 结构:
 *   连接 → 发 hello → 收数据(暂只打印日志, 命令解析后续再加).
 */

#include "ws_client.h"

#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"

/* 设备 ID 与服务器 URI (与合作者保持一致, 不要改动) */
#define DEVICE_ID     "esp32_001"
#define WEBSOCKET_URI "ws://123.56.216.186:2024/device/ws/esp32_001"

static const char *TAG = "ws_client";
static esp_websocket_client_handle_t s_client;
static ws_message_handler_t s_message_handler;

/** 发送一个 JSON 对象 (序列化后作为 WebSocket 文本帧发出) */
static void ws_client_send_json(cJSON *root)
{
    if (root == NULL) {
        return;
    }

    char *text = cJSON_PrintUnformatted(root);
    if (text == NULL) {
        cJSON_Delete(root);
        return;
    }

    /* 连接可能因长时间空闲/网络波动而断开, 这里最多重试 15 秒,
     * 等待 esp_websocket_client 自动重连 (3 秒一次) 后再发, 避免消息被悄悄丢弃. */
    int sent = 0;
    for (int i = 0; i < 15 && !sent; i++) {
        if (s_client != NULL && esp_websocket_client_is_connected(s_client)) {
            int r = esp_websocket_client_send_text(s_client, text, strlen(text), portMAX_DELAY);
            sent = (r >= 0);
        }
        if (!sent) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    if (sent) {
        ESP_LOGI(TAG, "sent: %s", text);
    } else {
        ESP_LOGW(TAG, "give up sending, websocket not connected after retries");
    }

    cJSON_free(text);
    cJSON_Delete(root);
}

/** 连接成功后上报身份 (hello) */
static void ws_client_send_hello(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddStringToObject(root, "device_id", DEVICE_ID);
    cJSON_AddStringToObject(root, "firmware", "dough-mixer");
    ws_client_send_json(root);
}

void ws_client_set_message_handler(ws_message_handler_t handler)
{
    s_message_handler = handler;
}

void ws_client_send_ack(const char *command_id, const char *status, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "ack");
    cJSON_AddStringToObject(root, "command_id", command_id ? command_id : "");
    cJSON_AddStringToObject(root, "device_id", DEVICE_ID);
    cJSON_AddStringToObject(root, "status", status ? status : "error");
    cJSON_AddStringToObject(root, "message", message ? message : "");
    ws_client_send_json(root);
}

void ws_client_send_event(const char *event, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "event");
    cJSON_AddStringToObject(root, "device_id", DEVICE_ID);
    cJSON_AddStringToObject(root, "event", event ? event : "");
    cJSON_AddStringToObject(root, "message", message ? message : "");
    ws_client_send_json(root);
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket connected");
        ws_client_send_hello();
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WebSocket disconnected");
        break;
    case WEBSOCKET_EVENT_DATA:
        if (data->op_code == 0x1 && data->data_ptr != NULL) {
            ESP_LOGI(TAG, "Received text: %.*s", data->data_len, (char *)data->data_ptr);
            if (s_message_handler != NULL) {
                s_message_handler((const char *)data->data_ptr, data->data_len);
            }
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket error");
        break;
    default:
        break;
    }
}

esp_err_t ws_client_start(void)
{
    esp_websocket_client_config_t websocket_cfg = {
        .uri = WEBSOCKET_URI,
        .reconnect_timeout_ms = 3000,
        .network_timeout_ms = 10000,
    };

    s_client = esp_websocket_client_init(&websocket_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "esp_websocket_client_init failed");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_websocket_register_events(s_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL));
    ESP_LOGI(TAG, "Connecting to %s", WEBSOCKET_URI);
    return esp_websocket_client_start(s_client);
}
