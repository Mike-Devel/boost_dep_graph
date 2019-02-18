cmake_minimum_required(VERSION 3.10)


# search for test source files (have to start with prefix test_)
file(GLOB_RECURSE TEST_FILES test_*.cpp)
add_executable(
	boost_dep_graph_tests
	main.cpp
	${TEST_FILES}
)

# include "libs" directory for #incldue <catch2/catch2>
target_include_directories(boost_dep_graph_tests PRIVATE libs)

target_link_libraries(
	boost_dep_graph_tests
PRIVATE
	MDev::bdg_core
)


## Make ctest run build.
# idea taken from https://stackoverflow.com/questions/733475/cmake-ctest-make-test-doesnt-build-tests
# TODO: DOES NOT WORK with MSVC open folder (${CMAKE_COMMAND} seems to be the problem, but a plain "cmake" doesn't pass the correct incldue directories)

ADD_TEST(NAME build_boost_dep_graph_tests COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target boost_dep_graph_tests)

include(libs/catch2/ParseAndAddCatchTests.cmake)

ParseAndAddCatchTests(boost_dep_graph_tests)


