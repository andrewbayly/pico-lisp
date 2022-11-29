cmake_minimum_required(VERSION 3.13)
include(/users/andrew/pico/PicoDVI/software/pico_sdk_import.cmake)

project(picodvi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(host_cdc_msc_hid)

add_compile_options(-Wall)
set_source_files_properties(mal.cpp PROPERTIES COMPILE_OPTIONS "-fexceptions")


# To change the default serial config for all apps, pass e.g.
#   cmake -DDVI_DEFAULT_SERIAL_CONFIG=pimoroni_demo_hdmi_cfg ..
# then rebuild. This is applied via target_compile_definitions on each app
# individually.
set(DVI_DEFAULT_SERIAL_CONFIG "pico_sock_cfg" CACHE STRING
	"Select a default pin configuration from common_dvi_pin_configs.h")

include_directories(
	assets
	include
	)

add_subdirectory(/Users/andrew/pico/PicoDVI/software/libdvi libdvi)
add_subdirectory(/Users/andrew/pico/PicoDVI/software/libsprite libsprite)

#add_subdirectory(apps)


# Example source
target_sources(host_cdc_msc_hid PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/hid_app.c
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/msc_app.c
        ${CMAKE_CURRENT_LIST_DIR}/key_pressed.c
        ${CMAKE_CURRENT_LIST_DIR}/mal.cpp
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(host_cdc_msc_hid PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_host
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(host_cdc_msc_hid PUBLIC pico_stdlib tinyusb_host tinyusb_board
	pico_stdlib
	pico_multicore
	pico_util
	libdvi
	libsprite
)

pico_add_extra_outputs(host_cdc_msc_hid)

# add url via pico_set_program_url
example_auto_set_url(host_cdc_msc_hid)




