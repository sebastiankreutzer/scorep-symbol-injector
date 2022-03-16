cmake_minimum_required(VERSION 3.15)
project(scorep-symbol-injector VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(AddLLVM)

include(llvm-lit)

set(SCOREP_PATH "" CACHE PATH "Path to ScoreP installation")

find_library(scorep_mgmt scorep_adapter_compiler_mgmt HINTS ${SCOREP_PATH}/lib REQUIRED)
find_library(scorep_measurement scorep_measurement HINTS ${SCOREP_PATH}/lib REQUIRED)

message(STATUS "Score-P libs: ${scorep_mgmt} ${scorep_measurement}")

add_subdirectory(lib)

enable_testing()
add_subdirectory(test)