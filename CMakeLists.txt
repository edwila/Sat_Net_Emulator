cmake_minimum_required(VERSION 3.15)
project(Emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(DEPENDENCY_SOURCES
    Dependencies/optimal.cc 
)

set(SATELLITE_SOURCES
    Helpers/Satellites/satellite_main.cc
    Helpers/Satellites/Satellites.cc
    ${DEPENDENCY_SOURCES} 
)

set(USER_SOURCES
    Helpers/Users/user_main.cc
    Helpers/Users/Users.cc
    ${DEPENDENCY_SOURCES}
)

add_library(shared_interface INTERFACE)
target_include_directories(shared_interface INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies
)

add_executable(station_driver Helpers/Stations/Station.cc)
add_executable(satellite_worker ${SATELLITE_SOURCES})
add_executable(user_worker ${USER_SOURCES})

set(EMULATOR_TARGETS station_driver satellite_worker user_worker)

foreach(TARGET_NAME IN LISTS EMULATOR_TARGETS)
    target_link_libraries(${TARGET_NAME} PRIVATE shared_interface rt pthread) 
endforeach()