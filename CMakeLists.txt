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
