# pico-idf project.cmake — ESP-IDF-shaped project bootstrap for Pico SDK.
#
# A project CMakeLists.txt looks like:
#   cmake_minimum_required(VERSION 3.13)
#   include($ENV{PIDF_PATH}/tools/cmake/project.cmake)
#   project(blink)

if(NOT DEFINED PIDF_PATH)
    if(DEFINED ENV{PIDF_PATH})
        set(PIDF_PATH "$ENV{PIDF_PATH}")
    else()
        get_filename_component(PIDF_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    endif()
endif()

if(NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH} AND EXISTS "/opt/pico-sdk")
    set(PICO_SDK_PATH "/opt/pico-sdk")
endif()

if(NOT DEFINED FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH} AND EXISTS "/opt/FreeRTOS-Kernel")
    set(FREERTOS_KERNEL_PATH "/opt/FreeRTOS-Kernel")
endif()

# Infer PICO_PLATFORM before the SDK/FreeRTOS imports so the correct
# FreeRTOS port (RP2040 / RP2350 ARM / RP2350 RISC-V) is selected.
if(NOT PICO_PLATFORM)
    if(DEFINED PIDF_TARGET AND PIDF_TARGET STREQUAL "pico2_w_riscv")
        set(PICO_PLATFORM "rp2350-riscv")
    elseif(DEFINED PICO_BOARD AND PICO_BOARD MATCHES "pico2")
        set(PICO_PLATFORM "rp2350")
    else()
        set(PICO_PLATFORM "rp2040")
    endif()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include("${PIDF_PATH}/cmake/pico_sdk_import.cmake")

set(FREERTOS_CONFIG_FILE_DIRECTORY "${PIDF_PATH}/components/freertos/include" CACHE PATH
    "Directory containing FreeRTOSConfig.h")

function(idf_component_register)
    cmake_parse_arguments(ARG "" "NAME" "SRCS;INCLUDE_DIRS;REQUIRES;PRIV_REQUIRES;PRIV_INCLUDE_DIRS" ${ARGN})
    get_filename_component(_dirname "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    if(ARG_NAME)
        set(_comp "${ARG_NAME}")
    else()
        set(_comp "${_dirname}")
    endif()
    if(NOT ARG_SRCS)
        add_library(${_comp} INTERFACE)
        if(ARG_INCLUDE_DIRS)
            target_include_directories(${_comp} INTERFACE ${ARG_INCLUDE_DIRS})
        endif()
        if(ARG_REQUIRES)
            target_link_libraries(${_comp} INTERFACE ${ARG_REQUIRES})
        endif()
        return()
    endif()
    add_library(${_comp} STATIC ${ARG_SRCS})
    if(ARG_INCLUDE_DIRS)
        target_include_directories(${_comp} PUBLIC ${ARG_INCLUDE_DIRS})
    endif()
    if(ARG_PRIV_INCLUDE_DIRS)
        target_include_directories(${_comp} PRIVATE ${ARG_PRIV_INCLUDE_DIRS})
    endif()
    # Headers only — do not pull pico_stdlib sources into every component.
    target_link_libraries(${_comp} PUBLIC pico_base_headers)
    if(ARG_REQUIRES)
        target_link_libraries(${_comp} PUBLIC ${ARG_REQUIRES})
    endif()
    if(ARG_PRIV_REQUIRES)
        target_link_libraries(${_comp} PRIVATE ${ARG_PRIV_REQUIRES})
    endif()
endfunction()

macro(project name)
    # FreeRTOS-Kernel and the Pico SDK also call project(). Only the
    # application project should bootstrap pico-idf.
    if(PIDF_PROJECT_READY)
        # Keep CMake happy if a nested CMakeLists expects `project()`.
    else()
    set(PIDF_PROJECT_READY TRUE)
    _project(${name} C CXX ASM)
    pico_sdk_init()

    # FreeRTOS port after the SDK is up (needs PICO_PLATFORM + SDK headers).
    include("${PIDF_PATH}/cmake/FreeRTOS_Kernel_import.cmake")

    set(PIDF_PROJECT_NAME ${name})

    add_subdirectory("${PIDF_PATH}/components/esp_common" "${CMAKE_BINARY_DIR}/pidf/esp_common")
    add_subdirectory("${PIDF_PATH}/components/log" "${CMAKE_BINARY_DIR}/pidf/log")
    add_subdirectory("${PIDF_PATH}/components/freertos" "${CMAKE_BINARY_DIR}/pidf/freertos")
    add_subdirectory("${PIDF_PATH}/components/esp_event" "${CMAKE_BINARY_DIR}/pidf/esp_event")
    add_subdirectory("${PIDF_PATH}/components/driver" "${CMAKE_BINARY_DIR}/pidf/driver")

    # Optional stacks (claw, later wifi, …) register here but stay out of
    # the default link line. EXCLUDE_FROM_ALL keeps blink from compiling
    # them until main REQUIRES the target.
    set(_pidf_core_comps esp_common log freertos esp_event driver)
    file(GLOB _pidf_comp_entries LIST_DIRECTORIES true "${PIDF_PATH}/components/*")
    foreach(_dir IN LISTS _pidf_comp_entries)
        if(IS_DIRECTORY "${_dir}" AND EXISTS "${_dir}/CMakeLists.txt")
            get_filename_component(_name "${_dir}" NAME)
            list(FIND _pidf_core_comps "${_name}" _found)
            if(_found EQUAL -1)
                add_subdirectory("${_dir}" "${CMAKE_BINARY_DIR}/pidf/${_name}" EXCLUDE_FROM_ALL)
            endif()
        endif()
    endforeach()

    if(EXISTS "${CMAKE_SOURCE_DIR}/main/CMakeLists.txt")
        add_subdirectory("${CMAKE_SOURCE_DIR}/main" "${CMAKE_BINARY_DIR}/main")
        # ESP-IDF apps include public headers without listing every
        # component; expose the foundation libs to `main`.
        target_link_libraries(main PUBLIC
            pidf_freertos driver esp_event log esp_common
            FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)
    endif()

    add_executable(${name} "${PIDF_PATH}/components/freertos/app_startup.c")
    target_include_directories(${name} PRIVATE ${FREERTOS_CONFIG_FILE_DIRECTORY})
    target_link_libraries(${name} PRIVATE
        main
        pidf_freertos
        driver
        esp_event
        log
        esp_common
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
        pico_stdlib
    )

    # W-board LED lives on the CYW43439. Use the no-lwIP arch so blink
    # does not need a full TCP stack. The planned `wifi` feature will
    # switch apps to pico_cyw43_arch_lwip_sys_freertos.
    if(PICO_CYW43_SUPPORTED)
        target_link_libraries(${name} PRIVATE pico_cyw43_arch_none)
        target_compile_definitions(${name} PRIVATE PIDF_CYW43=1)
    endif()

    set(_sdkconfig "${CMAKE_SOURCE_DIR}/sdkconfig")
    if(NOT EXISTS "${_sdkconfig}")
        set(_sdkconfig "${CMAKE_SOURCE_DIR}/sdkconfig.defaults")
    endif()
    if(EXISTS "${_sdkconfig}")
        file(STRINGS "${_sdkconfig}" _cfg_lines)
        foreach(_line IN LISTS _cfg_lines)
            if(_line MATCHES "^CONFIG_[A-Za-z0-9_]+=.*")
                target_compile_definitions(${name} PRIVATE "${_line}")
            endif()
        endforeach()
    endif()

    pico_enable_stdio_usb(${name} 1)
    pico_enable_stdio_uart(${name} 0)
    pico_add_extra_outputs(${name})
    endif()
endmacro()
