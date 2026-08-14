/**
 * @file    wifi.h
 * @brief   WiFi STA 模式 — 连接外部网络
 *
 * @section wifi_sta WiFi STA
 *   ESP32 作为 STA (station) 连接到能上网的 WiFi,
 *   连接成功后上层即可通过 WebSocket 访问云端服务器.
 *
 * @section flow 运行流程
 *   1. main 中调用 wifi_manager_start() 初始化并开始连接
 *   2. wifi_event_handler 处理 STA_START / DISCONNECTED / GOT_IP 事件
 *   3. 断线自动重连 (最多 10 次)
 *   4. (可选) wifi_manager_wait_connected() 阻塞等待拿到 IP
 */

#ifndef __WIFI_H__
#define __WIFI_H__

#include "esp_err.h"

/*====================================================================
 *  WiFi STA 配置 (占位 — 改成能上网的 WiFi)
 *====================================================================*/

/** 要连接的 WiFi 名称 (SSID) */
#define WIFI_SSID      "popkik"

/** WiFi 密码 (WPA2-PSK) */
#define WIFI_PASS      "123456789"

/*====================================================================
 *  API
 *====================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化并启动 WiFi STA 模式 (非阻塞)
 *
 * 内部调用链:
 *   1. nvs_flash_init()            — 初始化 NVS (存储 WiFi 配置, 幂等)
 *   2. esp_netif_init()            — 初始化 TCP/IP 协议栈
 *   3. esp_event_loop_create_default() — 创建系统事件循环
 *   4. esp_netif_create_default_wifi_sta() — 创建 STA 网络接口
 *   5. esp_wifi_init()             — 初始化 WiFi 驱动
 *   6. esp_wifi_set_mode(WIFI_MODE_STA)  — 设为 STA 模式
 *   7. esp_wifi_set_config() + esp_wifi_start() — 配置并启动
 *
 * @note 启动后 WiFi 在后台异步连接 (STA_START 事件里调用 esp_wifi_connect).
 *       上层可通过 wifi_manager_wait_connected() 等待连接结果.
 *
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t wifi_manager_start(void);

/**
 * @brief 阻塞等待 WiFi 连接成功或失败
 *
 * 等待事件组 WIFI_CONNECTED_BIT | WIFI_FAIL_BIT:
 *   - 拿到 IP → 返回 ESP_OK
 *   - 重连超过 10 次 → 返回 ESP_FAIL
 *
 * @return ESP_OK 已连接, ESP_FAIL 连接失败
 */
esp_err_t wifi_manager_wait_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_H__ */
