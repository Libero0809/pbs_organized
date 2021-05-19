cmake_minimum_required(VERSION 3.0)

project(erlay)
#set(CMAKE_CXX_FLAGS "-O3 -Wall")

add_definitions(-DDEBUG)
add_executable(Erlay Erlay.cpp)
target_link_libraries(Erlay minisketch xxhash)


add_definitions(-DCLION_DEBUG)
add_executable(Erlay_with_partition Erlay_with_partition.cpp)
target_link_libraries(Erlay_with_partition minisketch xxhash)

add_definitions(-DCLION_DEBUG)
add_executable(ErlayWPMatch Erlay_with_partition_match_ITRecon.cpp)
target_link_libraries(ErlayWPMatch minisketch xxhash)

