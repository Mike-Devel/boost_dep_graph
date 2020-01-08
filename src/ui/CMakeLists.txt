cmake_minimum_required(VERSION 3.10)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB BDG_UI_SOURCE_FILES "*.cpp" ".hpp")

add_library(bdg_ui ${BDG_UI_SOURCE_FILES})

target_include_directories(bdg_ui INTERFACE ..)

target_link_libraries( bdg_ui PUBLIC MDev::bdg_core Qt5::Core Qt5::Widgets Threads::Threads fmt::fmt fmt::fmt-header-only)

add_library(MDev::bdg_ui ALIAS bdg_ui)

