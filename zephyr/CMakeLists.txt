# SPDX-License-Identifier: GPL-3.0-only
if(CONFIG_LIGHT_COMMS)
  # Preserve the existing library identifier for downstream compatibility.
  zephyr_library_named(ieee802157_core)
  zephyr_library_sources(../src/occ.c ../src/packet.c)
  zephyr_library_sources_ifdef(CONFIG_LIGHT_COMMS_RECEIVER ../src/receiver.c)
  zephyr_library_sources_ifdef(CONFIG_LIGHT_COMMS_IMAGE ../src/image.c)
  zephyr_library_sources_ifdef(CONFIG_LIGHT_COMMS_PLAYER ../src/player.c)
  zephyr_library_sources_ifdef(CONFIG_LIGHT_COMMS_SPATIAL ../src/spatial.c)
  zephyr_library_sources_ifdef(CONFIG_LIGHT_COMMS_COOK ../src/cook.c)
  zephyr_library_sources_ifdef(CONFIG_LIGHT_COMMS_ROLLING ../src/rolling.c)
  zephyr_library_sources_ifdef(CONFIG_LIGHT_COMMS_SLOW_COLOUR ../src/colour.c)
  zephyr_include_directories(../include)
endif()
