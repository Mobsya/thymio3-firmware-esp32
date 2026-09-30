cmake_minimum_required(VERSION 3.5)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(NOT DEFINED ENV{ADF_PATH} OR "$ENV{ADF_PATH}" STREQUAL "")
    if(NOT DEFINED ENV{IDF_PATH} OR "$ENV{IDF_PATH}" STREQUAL "")
        message(FATAL_ERROR "ADF_PATH or IDF_PATH must be set. Run make setup, then make firmware.")
    endif()
    set(ENV{ADF_PATH} "$ENV{IDF_PATH}/..")
endif()

include($ENV{ADF_PATH}/CMakeLists.txt)
set(EXTRA_COMPONENTS_DIRS components/)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(thymio3-esp32-firmware)

# Include the prebuilt sounds in ESP-IDF flash metadata and release packaging.
set(AUDIO_TONE_BINARY "${CMAKE_BINARY_DIR}/audio_tone.bin")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/main/audio/audio_tone.bin"
               "${AUDIO_TONE_BINARY}" COPYONLY)
esptool_py_flash_to_partition(flash flash_tone "${AUDIO_TONE_BINARY}")

partition_table_add_check_size_target(audio_tone_check_size
    BINARY_PATH "${AUDIO_TONE_BINARY}"
    PARTITION_TYPE data
    PARTITION_SUBTYPE 0xff)
add_dependencies(app audio_tone_check_size)
add_dependencies(flash audio_tone_check_size)
