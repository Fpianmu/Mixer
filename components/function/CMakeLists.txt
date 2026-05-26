idf_component_register(
    SRCS "function.c"
    INCLUDE_DIRS "include"
    REQUIRES ledc_pwm gpiowork servo freertos stepper servo2 esc_controller
)