#include "ledc_pwm.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "ledc_pwm";

static ledc_channel_config_t *p_ch_cfg = NULL; // 保存通道配置以更新占空比
static ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;

esp_err_t pwm_init(int gpio_num, uint32_t freq_hz, uint32_t duty)
{
    // 1. 配置定时器
    ledc_timer_config_t timer_cfg = {
        .speed_mode = speed_mode,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT, // 固定13位分辨率，占空比 0~8191
        .freq_hz = freq_hz,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Timer config failed");
        return ret;
    }

    // 2. 配置通道
    ledc_channel_config_t ch_cfg = {
        .gpio_num = gpio_num,
        .speed_mode = speed_mode,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = duty,
        .hpoint = 0
    };
    ret = ledc_channel_config(&ch_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Channel config failed");
        return ret;
    }

    // 保存通道配置引用 (仅需要模式、通道和速度，用于后续更新占空比)
    static ledc_channel_config_t s_ch_cfg;
    s_ch_cfg = ch_cfg;
    p_ch_cfg = &s_ch_cfg;

    ESP_LOGI(TAG, "Initialized on GPIO %d, freq %lu Hz, duty %lu", gpio_num, freq_hz, duty);
    return ESP_OK;
}

esp_err_t ledc_pwm_set_duty(uint32_t duty)
{
    if (p_ch_cfg == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = ledc_set_duty(p_ch_cfg->speed_mode, p_ch_cfg->channel, duty);
    if (ret != ESP_OK) {
        return ret;
    }
    return ledc_update_duty(p_ch_cfg->speed_mode, p_ch_cfg->channel);
}