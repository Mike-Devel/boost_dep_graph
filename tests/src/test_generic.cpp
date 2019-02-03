#include <utils.hpp>

#include <catch2/catch.hpp>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

TEST_CASE( "sort_to_the_middle", "[boost_dep_graph_tests]" )
{
	std::vector<int> data( 20 );
	std::iota( data.begin(), data.end(), 0 );
	data.push_back( 0 );
	data.push_back( 4 );
	data.push_back( 4 );

	std::vector<int> ref{0, 2, 4, 4, 6, 8, 10, 12, 14, 16, 18, 19, 17, 15, 13, 11, 9, 7, 5, 4, 3, 1, 0};

	for( int i = 0; i < 100; ++i ) {
		std::shuffle( data.begin(), data.end(), std::random_device{} );
		mdev::sort_to_the_middle( data, std::less<>{} );

		CHECK( ref == data );
	}
}
