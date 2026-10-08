# SPDX-License-Identifier: GPL-3.0-only
zephyr_library_named(ieee802157_core)
zephyr_library_sources(../src/occ.c ../src/packet.c ../src/receiver.c ../src/image.c ../src/player.c)
zephyr_library_sources(../src/spatial.c)
zephyr_library_sources(../src/cook.c ../src/rolling.c)
zephyr_include_directories(../include)
