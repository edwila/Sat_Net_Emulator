cmake_minimum_required(VERSION 3.15)
project(Emulator)

# Force C++17 (Fixes the 'inline variables' error)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Output all compiled binaries to the build/bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SATELLITE_SOURCES
    Helpers/Satellites/satellite_main.cc
    Helpers/Satellites/Satellites.cc
)

set(USER_SOURCES
    Helpers/Users/user_main.cc
    Helpers/Users/Users.cc
    Helpers/Users/optimal.cc
)

add_library(shared_interface INTERFACE)
target_include_directories(shared_interface INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies
)

add_executable(station_driver Stations/Station.cc)
add_executable(satellite_worker ${SATELLITE_SOURCES})
add_executable(user_worker ${USER_SOURCES})

set(EMULATOR_TARGETS station_driver satellite_worker user_worker)

foreach(TARGET_NAME IN LISTS EMULATOR_TARGETS)
    # Standard dynamic linking. 'rt' and 'pthread' included for your upcoming Shared Memory!
    target_link_libraries(${TARGET_NAME} PRIVATE shared_interface rt pthread) 
endforeach()