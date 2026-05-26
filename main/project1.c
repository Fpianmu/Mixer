#include "ledc_pwm.h"
#include "gpiowork.h"
#include "servo.h"
#include "servo2.h"
#include "esc_controller.h"
#include "stepper.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "function.h"
#include <stdio.h>

static const char *TAG = "main";


void app_main(void)
{
    init_all();
    while (1) 
    {
        //servo_set_angle(10);
        //vTaskDelay(pdMS_TO_TICKS(500));
        //servo2_set_angle(45);
        //vTaskDelay(pdMS_TO_TICKS(500));
        //servo2_set_angle(0);
        //gpio_set(18,0);
        //vTaskDelay(pdMS_TO_TICKS(2000));
        //gpio_set(18,1);
        //vTaskDelay(pdMS_TO_TICKS(2000));
        //servo_set_angle(95);
        //vTaskDelay(pdMS_TO_TICKS(500));
        //servo2_set_angle(45);
        //vTaskDelay(pdMS_TO_TICKS(500));
        //servo2_set_angle(0);
        fwork(10000,5000,500,5000,3000);
    }
}