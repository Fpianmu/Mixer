idf_component_register(
    SRCS "function.c"
    INCLUDE_DIRS "include"
    REQUIRES ledc_pwm gpiowork servo freertos stepper servo2 function esc_controller IIC SPI FT6336U_TOUCH ST7789V_LCD ui HTTP LVGL_UI WIFI esp_wifi esp_event nvs_flash
)