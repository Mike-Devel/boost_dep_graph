cmake_minimum_required(VERSION 3.10)
project(boost_dep_graph  LANGUAGES CXX)

########## General Settings for the whole project ############################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Core CONFIG REQUIRED)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (MSVC)
	# Set warning level (CMAKE adds /W3 and msvc produces a warning, when we would just add /W4 )
	STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_compile_options(
		/permissive-
		/D_SCL_SECURE_NO_WARNINGS
		/D_CRT_SECURE_NO_WARNINGS
		/D_SILENCE_PARALLEL_ALGORITHMS_EXPERIMENTAL_WARNING
		/wd 4127
	)
else()
	add_compile_options(-Wall -Wextra)
endif()


file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.hpp")

add_library( boost_dep_graph_lib STATIC	${SOURCE_FILES}	)
add_library( MDev::boost_dep_graph_lib ALIAS boost_dep_graph_lib )

target_include_directories	( boost_dep_graph_lib INTERFACE	src )
target_link_libraries( boost_dep_graph_lib PUBLIC Qt5::Core Qt5::Widgets Threads::Threads)


######### generate the actual Executable

add_executable(	boost_dep_graph main.cpp )
target_link_libraries( boost_dep_graph MDev::boost_dep_graph_lib)

########## Testing ###########################################################
option(boost_dep_graph_INCLUDE_TESTS "Generate targets in test directory" ON)

if(boost_dep_graph_INCLUDE_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

