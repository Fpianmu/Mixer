idf_component_register(
    SRCS "http.c"
    INCLUDE_DIRS "include"
    REQUIRES function esp_http_server ledc_pwm gpiowork servo log freertos stepper servo2 esc_controller IIC SPI FT6336U_TOUCH ST7789V_LCD
    EMBED_FILES "http.html"       
)