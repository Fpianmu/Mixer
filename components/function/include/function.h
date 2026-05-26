#ifndef FUNCTION_H
#define FUNCTION_H

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

#ifdef __cplusplus
extern "C" {
#endif

void init_all(void);
void fwork(int duration1,int duration2,int duration3,int duration4,int duration5);

#ifdef __cplusplus
}
#endif

#endif