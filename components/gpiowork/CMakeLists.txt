idf_component_register(
    SRCS "gpiowork.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio    # 依赖 GPIO 驱动
)