/**
 * @file    wifi.c
 * @brief   WiFi STA 模式实现 — 连接外部网络
 *
 * @section arch 架构
 *   ESP32 作为 STA (station) 连接到能上网的 WiFi,
 *   成功后上层即可通过 WebSocket 访问云端服务器.
 *
 * @section retry 断线重连
 *   连接失败/断线时自动重连, 最多 10 次. 拿到 IP 后清零重试计数,
 *   并通过事件组通知上层 (wifi_manager_wait_connected).
 */

#include "wifi.h"

#include <string.h>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define WIFI_MAX_RETRY 10

static const char *TAG = "wifi_manager";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num;

/**
 * @brief WiFi 事件回调
 *
 * 处理三类事件:
 *   - WIFI_EVENT_STA_START: 驱动就绪 → 发起连接
 *   - WIFI_EVENT_STA_DISCONNECTED: 断线 → 未超重试次数则重连, 否则置失败位
 *   - IP_EVENT_STA_GOT_IP: 拿到 IP → 清零重试计数, 置连接成功位
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (s_retry_num < WIFI_MAX_RETRY) {
            s_retry_num++;
            ESP_LOGW(TAG, "Wi-Fi disconnected, retrying %d/%d", s_retry_num, WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_manager_start(void)
{
    /* NVS 初始化 (存储 WiFi 配置), 已初始化时幂等 */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs_flash_init failed");

    /* 事件组: 用于 wifi_manager_wait_connected() 通知连接结果 */
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "esp_event_loop_create_default failed");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "esp_wifi_init failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL),
        TAG,
        "register WIFI_EVENT handler failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL),
        TAG,
        "register IP_EVENT handler failed");

    /* 配置 STA 参数 (SSID / 密码) */
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "esp_wifi_set_mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "esp_wifi_set_config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start failed");

    ESP_LOGI(TAG, "wifi_manager_start finished, connecting to SSID:%s", WIFI_SSID);
    return ESP_OK;
}

esp_err_t wifi_manager_wait_connected(void)
{
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", WIFI_SSID);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "failed to connect to SSID:%s", WIFI_SSID);
    return ESP_FAIL;
}
