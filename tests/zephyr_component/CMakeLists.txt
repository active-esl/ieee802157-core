# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.20)
get_filename_component(OCC_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
list(APPEND ZEPHYR_EXTRA_MODULES "${OCC_CORE_DIR}")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(light_comms_selection_test)
target_sources(app PRIVATE src/main.c)
