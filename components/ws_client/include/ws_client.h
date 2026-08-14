/**
 * @file    ws_client.h
 * @brief   WebSocket 客户端 — 连接云端服务器
 *
 * 照合作者 01_led 的 ws_client 结构: 连接成功后发 hello,
 * 断线自动重连. 命令解析后续再加.
 */

#ifndef __WS_CLIENT_H_
#define __WS_CLIENT_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动 WebSocket 客户端 (非阻塞)
 *
 * 连接 ws://<服务器>:2024/device/ws/<device_id>,
 * 断线后每 3 秒自动重连, 连上后发送 hello 消息.
 *
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t ws_client_start(void);

/** WebSocket 文本消息回调: 收到命令文本时被调用 (payload 不保证 NULL 结尾, len 为长度) */
typedef void (*ws_message_handler_t)(const char *payload, int len);

/**
 * @brief 注册 WebSocket 文本消息回调
 *
 * 收到服务器下发的命令(文本帧)时, ws_client 会调用该回调.
 * 应在 ws_client_start() 之前注册.
 */
void ws_client_set_message_handler(ws_message_handler_t handler);

/**
 * @brief 向服务器发送命令应答 (ack)
 *
 * @param command_id 对应命令的 command_id
 * @param status     "ok" 或 "error"
 * @param message    结果说明 (可为空)
 */
void ws_client_send_ack(const char *command_id, const char *status, const char *message);

/**
 * @brief 向服务器发送一条事件通知 (例如长流程完成)
 *
 * 发送的消息格式:
 *   {"type":"event","device_id":"esp32_001","event":"<event>","message":"<message>"}
 *
 * @param event   事件名, 例如 "mix_done" / "push_done"
 * @param message 附带说明 (可为空)
 */
void ws_client_send_event(const char *event, const char *message);

#ifdef __cplusplus
}
#endif

#endif
