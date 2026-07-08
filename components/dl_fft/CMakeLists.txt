idf_component_register(
    SRCS dl_fft_f32.c dl_fft_s16.c dl_rfft_f32.c dl_rfft_s16.c
    INCLUDE_DIRS .
    REQUIRES esp-dsp
)

# Add esp-dsp base files that dl_fft needs
file(GLOB ESP_DSP_BASE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../esp-dsp/base/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../esp-dsp/base/isa/${CONFIG_IDF_TARGET}/*.S
)
target_sources(${COMPONENT_LIB} PRIVATE ${ESP_DSP_BASE_SRCS})
target_include_directories(${COMPONENT_LIB} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../esp-dsp/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../esp-dsp/base/isa/${CONFIG_IDF_TARGET}
)
