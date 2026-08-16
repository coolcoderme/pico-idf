idf_component_register(SRCS "esp_log.c" INCLUDE_DIRS "include" REQUIRES esp_common)
target_link_libraries(log PUBLIC pico_time)
