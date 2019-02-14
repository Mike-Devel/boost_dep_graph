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


template<typename T>
int sign( T val )
{
	return ( T( 0 ) < val ) - ( val < T( 0 ) );
}


template<class T>
struct span {
	span() = default;

	template<class Rng>
	span( Rng& r )
		: _start( r.data() )
		, _size( r.size() )
	{
	}
	std::size_t size() const { return _size; }

	T* begin() const { return _start; }
	T* end() const { return _start + _size; }

private:
	T*          _start;
	std::size_t _size;
};

} // namespace mdev