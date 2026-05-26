#ifndef LEDC_PWM_H
#define LEDC_PWM_H

#include "driver/ledc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 PWM 驱动
 * @param gpio_num   LED 连接的 GPIO 编号 
 * @param freq_hz    PWM 频率，单位 Hz (如 5000)
 * @param duty       初始占空比 (0 到 (2^duty_resolution - 1))
 * @return esp_err_t ESP_OK 成功，否则失败
 */
esp_err_t pwm_init(int gpio_num, uint32_t freq_hz, uint32_t duty);

/**
 * @brief 更新 PWM 占空比
 * @param duty 新的占空比
 * @return esp_err_t
 */
esp_err_t ledc_pwm_set_duty(uint32_t duty);

#ifdef __cplusplus
}
#endif

#endif