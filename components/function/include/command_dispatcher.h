/**
 * @file    command_dispatcher.h
 * @brief   服务器命令分发 — 把 WebSocket 收到的命令映射到面团机动作
 *
 * 服务器下发命令的 JSON 格式:
 *   {"command_id":"xxx","action":"start","weight":200}
 *   {"command_id":"xxx","action":"stop"}
 *   {"command_id":"xxx","action":"push_out"}
 *   {"command_id":"xxx","action":"push_back"}
 *
 * action 映射到已有的 function 函数 (函数名不改):
 *   start      → weight_work(weight)
 *   stop       → fstop()
 *   push_out   → push_and_out(1)
 *   push_back  → push_and_out(0)
 */

#ifndef __COMMAND_DISPATCHER_H_
#define __COMMAND_DISPATCHER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 处理一条服务器命令 (JSON 文本)
 *
 * 解析命令并分发到对应动作, 处理完通过 ws_client_send_ack 回执.
 * 长耗时动作(和面/出面)会在独立任务里执行, 不阻塞调用者.
 *
 * @param payload     收到的 JSON 文本 (不保证 NULL 结尾)
 * @param payload_len 文本长度
 */
void command_dispatcher_handle(const char *payload, int payload_len);

#ifdef __cplusplus
}
#endif

#endif /* __COMMAND_DISPATCHER_H_ */
