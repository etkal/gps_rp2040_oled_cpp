cmake_minimum_required(VERSION 3.24)

# Set the board type for pin and LED decisions
set(PICO_BOARD pico)
#set(PICO_BOARD pico_w)
#set(PICO_BOARD waveshare_rp2040_zero)
#set(PICO_BOARD seeed_xiao_rp2040)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
#include(pico_libs_import.cmake)

project(gps_oled C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(gps_oled)
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
# generate the header file into the source tree as it is included in the RP2040 datasheet
pico_generate_pio_header(gps_oled ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

add_subdirectory(src)
        
target_include_directories(gps_oled PUBLIC
    ${CMAKE_CURRENT_LIST_DIR})

if (PICO_BOARD STREQUAL pico_w)
  target_link_libraries(gps_oled pico_stdlib pico_stdio hardware_gpio hardware_pio hardware_i2c pico_cyw43_arch_none)
else()
  target_link_libraries(gps_oled pico_stdlib pico_stdio hardware_gpio hardware_pio hardware_i2c)
endif()

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(gps_oled)
