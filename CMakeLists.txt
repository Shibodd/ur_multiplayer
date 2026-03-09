cmake_minimum_required (VERSION 3.21)

project (ur_multiplayer LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Ice REQUIRED CONFIG)
find_package (SFML REQUIRED COMPONENTS graphics window system)

include (idl/sources.cmake)

add_library (clock_sync STATIC src/clock_offset_calculator.cpp)
target_include_directories (clock_sync PUBLIC include)

add_library (server STATIC src/server.cpp src/lobby.cpp src/race_control.cpp ${IDL_MESSAGE_SOURCES})
slice2cpp_generate (server)
target_link_libraries (server Ice::Ice clock_sync)
target_include_directories (server PUBLIC include ${CMAKE_BINARY_DIR}/generated/server)

add_library (rc_client STATIC src/race_control_client_servant.cpp ${IDL_MESSAGE_SOURCES})
slice2cpp_generate (rc_client)
target_link_libraries (rc_client Ice::Ice)
target_include_directories (rc_client PUBLIC include ${CMAKE_BINARY_DIR}/generated/rc_client)

add_library (participant STATIC src/participant.cpp ${IDL_MESSAGE_SOURCES})
slice2cpp_generate (participant)
target_link_libraries (participant Ice::Ice clock_sync)
target_include_directories (participant PUBLIC include ${CMAKE_BINARY_DIR}/generated/participant)

add_executable (demo_client demo/client.cpp)
target_link_libraries (demo_client participant sfml-graphics sfml-window sfml-system)

add_executable (demo_server demo/server.cpp)
target_link_libraries (demo_server server)

add_executable (demo_rc_client demo/rc_client.cpp)
target_link_libraries (demo_rc_client rc_client)