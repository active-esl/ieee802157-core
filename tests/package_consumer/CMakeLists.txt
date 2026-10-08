# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(light_comms_package_consumer LANGUAGES C)
find_package(light_comms_core 0.1 CONFIG REQUIRED)
add_executable(package_consumer main.c)
target_link_libraries(package_consumer PRIVATE light_comms::occ)
