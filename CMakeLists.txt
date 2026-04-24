cmake_minimum_required(VERSION 3.13)
project(Emulator)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(STATION_SOURCES
    Stations/Runner.cc
    Helpers/Satellites.cc
)

add_library(shared_interface INTERFACE)
target_include_directories(shared_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

add_executable(station ${STATION_SOURCES})

if(WIN32)
    target_link_options(station PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_libraries(station PRIVATE shared_interface ws2_32 winmm)
else()
    target_link_libraries(station PRIVATE shared_interface)
endif()