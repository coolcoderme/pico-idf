cmake_minimum_required(VERSION 3.13)

# Fall back to the SDK installed by .cursor/install.sh so a plain
# `cmake -S . -B build` works without exporting PICO_SDK_PATH by hand.
if(NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH} AND EXISTS "/opt/pico-sdk")
    set(PICO_SDK_PATH "/opt/pico-sdk")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in the Raspberry Pi Pico SDK (must be done before project()).
include(pico_sdk_import.cmake)

project(pico_idf C CXX ASM)

# Initialize the SDK.
pico_sdk_init()

add_executable(blink
    src/main.c
)

target_link_libraries(blink pico_stdlib)

# Route stdio over USB CDC (so `printf` shows up on the USB serial console)
# and turn off the default UART stdio.
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)

# Generate the .uf2 / .bin / .hex / .map artifacts alongside the ELF.
pico_add_extra_outputs(blink)
