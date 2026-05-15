cmake_minimum_required(VERSION 3.5)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(ENV{ADF_PATH} "$ENV{IDF_PATH}/..")
include ($ENV{ADF_PATH}/CMakeLists.txt)
set(EXTRA_COMPONENTS_DIRS components/)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(thymio3-esp32-firmware)
