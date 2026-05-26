idf_component_register(
    SRCS "project1.c"
    INCLUDE_DIRS "."
    REQUIRES ledc_pwm gpiowork servo freertos stepper servo2 esc_controller function
)
