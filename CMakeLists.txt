cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENTS_DIRS components/)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(ENV{ADF_PATH} "/home/stefano/Desktop/esp-adf-2.4")
include ($ENV{ADF_PATH}/CMakeLists.txt)
project(thymio3-esp32-firmware)
