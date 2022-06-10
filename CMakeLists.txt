cmake_minimum_required(VERSION 3.12)

project(abuseipdb-client LANGUAGES CXX VERSION 1.0.0 DESCRIPTION "An AbuseIPDB frontend for Linux and server administrators")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-Dabuseipdb_DEBUG)
    set(VERSION_SUFFIX "-debug")
else()
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cfg/extract_cfg.cmake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/resources/Version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/resources/Version.hpp @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/resources/Resources.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/resources/Resources.hpp @ONLY)

file(GLOB_RECURSE FILES FOLLOW_SYMLINKS ${CMAKE_CURRENT_SOURCE_DIR}/src src/*.cpp)

include_directories(
    include/
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conan.cmake)
    message(STATUS "Download conan installer. Please wait...")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/conan.cmake)

conan_cmake_run(
    REQUIRES
        spdlog/1.10.0
        nlohmann_json/3.10.5
        libcurl/7.83.1
    BASIC_SETUP
)

add_executable(
    ${PROJECT_NAME}

    ${FILES}
)

target_link_libraries(
    ${PROJECT_NAME}

    ${CONAN_LIBS}
)

add_library(
    ${PROJECT_NAME}_shared
    SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/AbuseIpDbApi.cpp
)

target_link_libraries(
    ${PROJECT_NAME}_shared

    ${CONAN_LIBS}
)

add_library(
    ${PROJECT_NAME}_static
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/AbuseIpDbApi.cpp
)

target_link_libraries(
    ${PROJECT_NAME}_static

    ${CONAN_LIBS}
)