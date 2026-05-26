#include "esp_log.h"
#include "esp_err.h"
#include "ledc_pwm.h"
#include "gpiowork.h"
#include "servo.h"
#include "servo2.h"
#include "esc_controller.h"
#include "stepper.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "function";

esc_handle_t esc1,esc2;

void init_all()
{
    // 启动 GPIO  
    gpio_set(7,0); //水泵
    gpio_set(16,0); //研磨
    gpio_set(18,0); //面粉磁铁
    //初始化舵机
    servo_init(8); //旋转
    servo2_init(3); //击打
    //步进电机初始化
    stepper_config_t config = 
    {
        .en_pin = 6,
        .dir_pin = 5,
        .stp_pin = 4,
        .freq_hz = 1000,
        .duration_ms = 5000   // 仅用于 stepper_start 的默认值
    };
    stepper_init(&config);
    //电调初始化
     // 初始化电调1：GPIO 8，使用通道 6
    esc_init(&esc1, 15, LEDC_CHANNEL_6);  //面粉
    // 初始化电调2：GPIO 9，使用通道 7
    esc_init(&esc2, 9, LEDC_CHANNEL_7);  //搅拌

    // 注意：必须在校准过程中（esc_init 函数内延时3秒时）
    // 手动为对应的电调上电，否则校准失败！

    vTaskDelay(pdMS_TO_TICKS(1000));
}
void fwork(int duration1,int duration2,int duration3,int duration4,int duration5)
{
    /*
    初始化全部完成
    开始工作
    */
   //Task1 加入面粉
    if (duration1 >=5000)
    {
        int n = duration1/5000;
        for (int i = 1;i <= n;i++)
        {
            esc_set_throttle(&esc1, 40.0); // 电调1 40%油门
            gpio_set(18,1);
            vTaskDelay(pdMS_TO_TICKS(5000));
            esc_set_throttle(&esc1, 0.0); // 电调1 熄火
            gpio_set(18,0);
            vTaskDelay(pdMS_TO_TICKS(2500));
        }
    }
    else
    {
         esc_set_throttle(&esc1, 40.0); // 电调1 40%油门
         gpio_set(18,1);
         vTaskDelay(pdMS_TO_TICKS(duration1));
         esc_set_throttle(&esc1, 0.0); // 电调1 熄火
         gpio_set(18,0);
    }
    //Task2 水泵工作
    gpio_set(7,1); //水泵
    vTaskDelay(pdMS_TO_TICKS(duration2));
    gpio_set(7,0); //水泵
    //Task3 研磨电机与加料舵机同时工作
    gpio_set(16,1); //研磨
    //加料1
    servo_set_angle(10);
    vTaskDelay(pdMS_TO_TICKS(500));
    servo2_set_angle(65);
    vTaskDelay(pdMS_TO_TICKS(duration3));
    servo2_set_angle(0);
    vTaskDelay(pdMS_TO_TICKS(500));
    //加料2
    servo_set_angle(100);
     vTaskDelay(pdMS_TO_TICKS(500));
    servo2_set_angle(60);
    vTaskDelay(pdMS_TO_TICKS(duration3));
    servo2_set_angle(0);
    vTaskDelay(pdMS_TO_TICKS(duration4));
    //研磨停止
    gpio_set(16,0); //研磨
    //Task4 关盖子搅拌
    // 方式1：使用动态函数，正转 3 秒，频率 1000Hz
    stepper_move_for(2900, 1000, 0);
    vTaskDelay(pdMS_TO_TICKS(3000)); // 等待运动结束

    esc_set_throttle(&esc2, 40.0); // 电调1 40%油门
    vTaskDelay(pdMS_TO_TICKS(duration5));
    esc_set_throttle(&esc2, 0.0); // 电调1 熄火

    // 方式2：反转 2 秒，频率 1000Hz
    stepper_move_for(2900, 1000, 1);
    vTaskDelay(pdMS_TO_TICKS(3000));
}
