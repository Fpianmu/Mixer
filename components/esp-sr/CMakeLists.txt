set(ESP_SR_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(ESP_SR_INCLUDE_DIRS
    ${ESP_SR_PATH}/include/${CONFIG_IDF_TARGET}
    ${ESP_SR_PATH}/src/include
)
file(GLOB_RECURSE ESP_SR_SRCS ${ESP_SR_PATH}/src/*.c)

idf_component_register(
    SRCS ${ESP_SR_SRCS}
    INCLUDE_DIRS ${ESP_SR_INCLUDE_DIRS}
    PRIV_INCLUDE_DIRS ${ESP_SR_INCLUDE_DIRS}
    REQUIRES json dl_fft esp-dsp esp_partition spiffs spi_flash nvs_flash
)

# Use add_prebuilt_library (ESP-IDF function) for prebuilt libs
file(GLOB ESP_SR_LIBS ${ESP_SR_PATH}/lib/${CONFIG_IDF_TARGET}/lib*.a)
set(LIB_NAMES "")
foreach(LIB ${ESP_SR_LIBS})
    get_filename_component(LIB_FULLNAME ${LIB} NAME_WE)
    string(SUBSTRING ${LIB_FULLNAME} 3 -1 SHORTNAME)
    add_prebuilt_library(${SHORTNAME} ${LIB} REQUIRES esp-sr)
    list(APPEND LIB_NAMES ${SHORTNAME})
endforeach()
target_link_libraries(${COMPONENT_LIB} PRIVATE ${LIB_NAMES})
