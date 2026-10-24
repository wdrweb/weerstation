# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(weerstation)

target_sources(app PRIVATE src/main.cpp src/storage.cpp src/sensor_manager.cpp src/web_server.cpp src/status_led.cpp src/wifi_sta.c)

zephyr_linker_sources(SECTIONS src/sections-rom.ld)

set(web_resource_dir src/static_web_resources)
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
foreach(web_resource index.html style.css main.js)
  generate_inc_file_for_target(app
    ${web_resource_dir}/${web_resource}
    ${gen_dir}/${web_resource}.inc)
endforeach()
