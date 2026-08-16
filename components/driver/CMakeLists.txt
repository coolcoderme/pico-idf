idf_component_register(SRCS "gpio.c" INCLUDE_DIRS "include" REQUIRES esp_common)

target_link_libraries(driver PUBLIC hardware_gpio)
if(PICO_CYW43_SUPPORTED)
    target_link_libraries(driver PUBLIC pico_cyw43_arch_none)
    target_compile_definitions(driver PUBLIC PIDF_CYW43=1)
endif()
