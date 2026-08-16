idf_component_register(
    NAME pidf_freertos
    INCLUDE_DIRS "include"
    REQUIRES log esp_common
)

target_include_directories(pidf_freertos INTERFACE ${FREERTOS_CONFIG_FILE_DIRECTORY})
target_link_libraries(pidf_freertos INTERFACE FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)
