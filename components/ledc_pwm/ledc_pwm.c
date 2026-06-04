#include "ledc_pwm.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stdbool.h>

static const char *TAG = "ledc_pwm";

/* DRV8870 最佳 PWM 频率: 50kHz, 13位分辨率 */
#define PWM_FREQ_HZ       50000
#define DUTY_RESOLUTION   LEDC_TIMER_13_BIT
#define DUTY_MAX          ((1 << LEDC_TIMER_13_BIT) - 1)  /* 8191   */
#define DUTY_50_PERCENT   (DUTY_MAX / 2)                  /* 4096   */
#define DUTY_OFF          0

#define CH0_GPIO          12
#define CH1_GPIO          13

static bool s_initialized = false;

static esp_err_t init(void)
{
    /* 1. 配置定时器 — 两路通道共用同一个定时器 */
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = DUTY_RESOLUTION,
        .freq_hz         = PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Timer config failed");
        return ret;
    }

    /* 2. 配置通道0 — GPIO 12 */
    ledc_channel_config_t ch0_cfg = {
        .gpio_num   = CH0_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = DUTY_OFF,
        .hpoint     = 0,
    };
    ret = ledc_channel_config(&ch0_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Channel 0 config failed");
        return ret;
    }

    /* 3. 配置通道1 — GPIO 13 */
    ledc_channel_config_t ch1_cfg = {
        .gpio_num   = CH1_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = DUTY_OFF,
        .hpoint     = 0,
    };
    ret = ledc_channel_config(&ch1_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Channel 1 config failed");
        return ret;
    }

    ESP_LOGI(TAG, "Initialized: GPIO%d (CH0), GPIO%d (CH1), %dHz",
             CH0_GPIO, CH1_GPIO, PWM_FREQ_HZ);
    return ESP_OK;
}

void ledc_pwm_set_state(int state)
{
    /* 首次调用时自动初始化 */
    if (!s_initialized) {
        esp_err_t ret = init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Init failed, cannot set state");
            return;
        }
        s_initialized = true;
    }

    uint32_t duty_ch0;
    uint32_t duty_ch1;

    if (state == 0) {
        /* 关闭: 两路均输出 0% */
        duty_ch0 = DUTY_OFF;
        duty_ch1 = DUTY_OFF;
        ESP_LOGI(TAG, "State: OFF (both 0%%)");
    } else {
        /* 开启: 通道0=50%, 通道1=0% */
        duty_ch0 = DUTY_50_PERCENT;
        duty_ch1 = DUTY_OFF;
        ESP_LOGI(TAG, "State: ON  (CH0=50%%, CH1=0%%)");
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_ch0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_ch1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}
