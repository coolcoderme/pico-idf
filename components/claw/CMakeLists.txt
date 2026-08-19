idf_component_register(
    SRCS
        "json_mini.c"
        "lock.c"
        "cap.c"
        "event.c"
        "memory.c"
        "core.c"
        "sched.c"
        "stubs.c"
        "mcp.c"
        "builtins.c"
        "runtime.c"
        "repl.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES esp_common log driver pidf_freertos
)

target_link_libraries(claw PRIVATE pico_stdio)
