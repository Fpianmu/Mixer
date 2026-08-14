/**
 * @file    command_dispatcher.c
 * @brief   服务器命令分发实现
 *
 * 解析 WebSocket 收到的 JSON 命令, 分发到 weight_work / fstop / push_and_out.
 * 和面(weight_work)与出面(push_and_out)耗时较长, 放到独立任务里执行,
 * 避免阻塞 WebSocket 事件任务 (否则会卡住心跳, 导致连接被服务器断开).
 */

#include "command_dispatcher.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "function.h"
#include "ws_client.h"

static const char *TAG = "command_dispatcher";

/** 后台执行和面 (weight_work), 重量由任务参数直接传入 */
static void mixer_task(void *arg)
{
    uint32_t weight = (uint32_t)(uintptr_t)arg;
    weight_work(weight);
    vTaskDelete(NULL);
}

/** 后台执行出面/退面 (push_and_out), 方向由任务参数传入: 1=出 0=退 */
static void stepper_task(void *arg)
{
    int direction = (int)(uintptr_t)arg;
    push_and_out(direction);
    vTaskDelete(NULL);
}

/** 安全拷贝 JSON 字符串字段到 dst (不足自动补 '\0') */
static void copy_json_string(char *dst, size_t dst_size, const cJSON *item)
{
    dst[0] = '\0';
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(dst, item->valuestring, dst_size - 1);
        dst[dst_size - 1] = '\0';
    }
}

void command_dispatcher_handle(const char *payload, int payload_len)
{
    if (payload == NULL || payload_len <= 0) {
        ws_client_send_ack("", "error", "empty payload");
        return;
    }

    /* WebSocket 收到的数据不保证 NULL 结尾, 拷贝一份再解析 */
    char *text = calloc(1, (size_t)payload_len + 1);
    if (text == NULL) {
        ws_client_send_ack("", "error", "out of memory");
        return;
    }
    memcpy(text, payload, (size_t)payload_len);

    cJSON *root = cJSON_Parse(text);
    free(text);
    if (root == NULL) {
        ws_client_send_ack("", "error", "invalid json");
        return;
    }

    char command_id[64];
    copy_json_string(command_id, sizeof(command_id),
                     cJSON_GetObjectItemCaseSensitive(root, "command_id"));

    const cJSON *action = cJSON_GetObjectItemCaseSensitive(root, "action");
    if (!cJSON_IsString(action) || action->valuestring == NULL) {
        cJSON_Delete(root);
        ws_client_send_ack(command_id, "error", "missing action");
        return;
    }

    const char *act = action->valuestring;

    if (strcmp(act, "start") == 0) {
        const cJSON *weight_item = cJSON_GetObjectItemCaseSensitive(root, "weight");
        if (!cJSON_IsNumber(weight_item)) {
            cJSON_Delete(root);
            ws_client_send_ack(command_id, "error", "missing weight");
            return;
        }
        uint32_t weight = (uint32_t)weight_item->valuedouble;
        ESP_LOGI(TAG, "action=start weight=%u", (unsigned)weight);
        ws_client_send_ack(command_id, "ok", "start accepted");
        xTaskCreate(mixer_task, "mixer_task", 8192, (void *)(uintptr_t)weight, 5, NULL);

    } else if (strcmp(act, "stop") == 0) {
        fstop();
        ESP_LOGI(TAG, "action=stop");
        ws_client_send_ack(command_id, "ok", "stop executed");

    } else if (strcmp(act, "push_out") == 0) {
        ESP_LOGI(TAG, "action=push_out");
        ws_client_send_ack(command_id, "ok", "push_out accepted");
        xTaskCreate(stepper_task, "stepper_task", 4096, (void *)(uintptr_t)1, 5, NULL);

    } else if (strcmp(act, "push_back") == 0) {
        ESP_LOGI(TAG, "action=push_back");
        ws_client_send_ack(command_id, "ok", "push_back accepted");
        xTaskCreate(stepper_task, "stepper_task", 4096, (void *)(uintptr_t)0, 5, NULL);

    } else {
        ESP_LOGW(TAG, "unknown action: %s", act);
        ws_client_send_ack(command_id, "error", "unknown action");
    }

    cJSON_Delete(root);
}
