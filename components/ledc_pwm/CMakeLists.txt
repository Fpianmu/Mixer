idf_component_register(
    SRCS "ledc_pwm.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_ledc  esp_driver_gpio        # 依赖 LEDC 和 GPIO 驱动
)