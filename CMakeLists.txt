cmake_minimum_required(VERSION 3.10)
project(boost_dep_graph  LANGUAGES CXX)

########## General Settings for the whole project ############################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if (MSVC)
	# Set warning level (CMAKE adds /W3 and msvc produces a warning, when we would just add /W4 )
	STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_compile_options(
		/permissive-
	)
else()
	add_compile_options(-Wall -Wextra)
endif()

########## Dependencies #######################################
find_package(Threads REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Core CONFIG REQUIRED)

########## Compile modules & libraries #######################################
add_subdirectory(src/ui)
add_subdirectory(src/core)


######### generate the executable ############################################

find_package(Qt5Core CONFIG REQUIRED)
add_executable(	boost_dep_graph main.cpp )

target_link_libraries( boost_dep_graph
	MDev::bdg_core
	MDev::bdg_ui
	Qt5::Core
)

########## Testing ###########################################################
option(boost_dep_graph_INCLUDE_TESTS "Generate targets in test directory" ON)

if(boost_dep_graph_INCLUDE_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

