set(ESP_AE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# Prebuilt library
set(AE_LIB ${ESP_AE_PATH}/lib/${CONFIG_IDF_TARGET}/libesp_audio_effects.a)

# Include gmf_fft sources directly (avoid component dependency issues)
file(GLOB GMF_SRCS ${ESP_AE_PATH}/../gmf_fft/src/common/*.c)
if(CONFIG_IDF_TARGET STREQUAL "esp32s3")
    list(APPEND GMF_SRCS ${ESP_AE_PATH}/../gmf_fft/src/esp32s3/fft_pie_radix2_dit_s16.S)
else()
    list(APPEND GMF_SRCS ${ESP_AE_PATH}/../gmf_fft/src/esp_c/gmf_fft_radix2_dit_s16.c)
endif()

idf_component_register(
    SRCS ${GMF_SRCS}
    INCLUDE_DIRS
        ${ESP_AE_PATH}/include
        ${ESP_AE_PATH}/../gmf_fft/include
    REQUIRES heap
)

# Link prebuilt audio effects lib
target_link_libraries(${COMPONENT_LIB} INTERFACE ${AE_LIB})
