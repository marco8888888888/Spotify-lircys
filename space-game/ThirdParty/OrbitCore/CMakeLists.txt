cmake_minimum_required(VERSION 3.16)
project(OrbitCore VERSION 0.1.0 LANGUAGES CXX)

option(ORBITCORE_BUILD_TESTS "Build OrbitCore unit tests" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(OrbitCore STATIC
  src/Kepler.cpp
  src/Orbit.cpp
  src/System.cpp
  src/Patched.cpp
)
add_library(OrbitCore::OrbitCore ALIAS OrbitCore)
target_include_directories(OrbitCore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(OrbitCore PUBLIC cxx_std_17)
set_target_properties(OrbitCore PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(OrbitCore PRIVATE /W4 /permissive-)
else()
  target_compile_options(OrbitCore PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion)
endif()

if(ORBITCORE_BUILD_TESTS)
  enable_testing()
  add_executable(OrbitCoreTests
    tests/main.cpp
    tests/test_kepler.cpp
    tests/test_orbit.cpp
    tests/test_system.cpp
  )
  target_include_directories(OrbitCoreTests PRIVATE tests)
  target_link_libraries(OrbitCoreTests PRIVATE OrbitCore::OrbitCore)
  set_target_properties(OrbitCoreTests PROPERTIES CXX_EXTENSIONS OFF)
  add_test(NAME OrbitCoreTests COMMAND OrbitCoreTests)
endif()
