cmake_minimum_required(VERSION 3.10)

file(GLOB SOURCE_FILES "*.cpp" "*.hpp")

add_library( bdg_core STATIC ${SOURCE_FILES})
add_library( MDev::bdg_core ALIAS bdg_core )

target_include_directories(bdg_core INTERFACE ..)

