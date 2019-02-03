#pragma once

#include <algorithm>
#include <vector>


namespace mdev {

template<class Rng, class Cmp>
void sort_to_the_middle( Rng& rng, Cmp cmp )
{
	std::sort( rng.begin(), rng.end(), cmp );
	for( int i = 0; i < ( rng.size() + 1 ) / 2; i += 1 ) {
		std::rotate( rng.begin() + i, rng.begin() + i + 1, rng.end() );
	}
	std::reverse( rng.begin() + rng.size() / 2, rng.end() );
}

template<class T>
std::vector<T> merge( std::vector<T> l, const std::vector<T>& r )
{
	l.insert( l.end(), r.begin(), r.end() );
	return l;
}

} // namespace mdev