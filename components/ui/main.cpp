/**
 * @file    main.cpp
 * @brief   和面机应用入口 — 初始化所有外设, 启动 UI 和服务
 *
 * 初始化顺序 (严格按依赖关系):
 *   1. NVS / 系统事件循环     — WiFi 和 HTTP 的前置依赖
 *   2. 电机 (step_init)       — GPIO + GPTimer + PWM + 继电器
 *   3. 串口 (uart_init+tasks) — UART0 指令监听
 *   4. WiFi AP                — 手机连接热点
 *   5. TCP 服务器             — 调试指令 (端口 8080)
 *   6. HTTP 服务器            — 网页控制面板 (端口 80)
 *   7. LCD (bsp_lcd_init)     — SPI + ST7789V 面板
 *   8. 触摸 (ft6336u_touch)   — I2C + FT6336U
 *   9. LVGL (lvgl_ui_init)    — LVGL 内核 + 注册显示和触摸
 *  10. EEZ UI (vars+ui_init)  — 加载 EEZ Studio 画面
 *
 * 主循环: ui_tick() 每 10ms 调用一次, 驱动 EEZ Flow 状态机.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "lcd.h"
#include "touch.h"
#include "lvgl_ui.h"
#include "ui.h"
#include "vars.h"
#include "motor.h"
// #include "uart.h"   // 串口已禁用
#include "wifi.h"
#include "http.h"

static const char *TAG = "MAIN";

extern "C" void app_main(void)
{
    /* ---- 系统基础 ---- */
    nvs_flash_init();                  /* NVS 存储 (WiFi 配置) */
    esp_event_loop_create_default();   /* 事件循环 (WiFi + HTTP 依赖) */

    /* ---- 电机 ---- */
    
    step_init();  /* GPIO + GPTimer + ESC PWM + 舵机 PWM + 继电器 */
    ESP_LOGI(TAG, "Motor init done");
    dough_esc_init();  /* 面粉电机 ESC 校准 (5% duty) */

    /* ---- 串口 (UART0, 115200bps) ---- */
    // uart_init();
    // xTaskCreate(uart_command_task,  "uart1_cmd", 4096, NULL, 10, NULL);
    // xTaskCreate(uart0_command_task, "uart0_cmd", 4096, NULL, 10, NULL);
    // ESP_LOGI(TAG, "UART init done");

    /* ---- WiFi AP + TCP 服务器 (8080) ---- */
    wifi_init_softap();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);

    /* ---- HTTP 服务器 (80, 网页控制面板) ---- */
    http_server_start();

    /* ---- LCD ---- */
    bsp_lcd_init();
    ESP_LOGI(TAG, "LCD init done");

    /* ---- 触摸 ---- */
    ft6336u_touch_init();

    /* ---- LVGL 内核 + 注册显示/触摸 ---- */
    lvgl_ui_init();

    /* ---- EEZ Studio UI ---- */
    lvgl_port_lock(0);
    vars_init();       /* 初始化全局变量 (dough_weight 默认 200g) */
    ui_init();         /* EEZ Flow 初始化 + 创建 6 屏幕 + 加载首页 */
    lvgl_port_unlock();

    ESP_LOGI(TAG, "System started, WiFi: dough_mixer / 12345678");

    //magnet_start();

    /* 主循环: 驱动 EEZ Flow 状态机 + LVGL 渲染 */
    while (1) {
        lvgl_port_lock(0);
        ui_tick();     /* EEZ Flow 步进: 处理动画/事件/属性绑定 */
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
