cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(ENV{ADF_PATH} "F:/_projects/Thymio3/esp-adf-gctronic_v2.4")
include ($ENV{ADF_PATH}/CMakeLists.txt)
project(thymio3-esp32-firmware)
