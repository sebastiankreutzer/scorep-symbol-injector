cmake_minimum_required(VERSION 3.17)
project(scorep-symbol-injector VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SCOREP_PATH "" CACHE PATH "Path to ScoreP installation")

find_library(scorep_mgmt scorep_adapter_compiler_mgmt HINTS ${SCOREP_PATH}/lib REQUIRED)
find_library(scorep_measurement scorep_measurement HINTS ${SCOREP_PATH}/lib REQUIRED)

message(STATUS "Score-P libs: ${scorep_mgmt} ${scorep_measurement}")

add_subdirectory(lib)
