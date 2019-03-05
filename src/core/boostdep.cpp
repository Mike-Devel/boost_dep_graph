#include "boostdep.hpp"

#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <set>
#include <vector>

#include <iostream>

//#define BDG_DONT_USE_STD_PARALLEL

// clang-format off
#ifndef BDG_DONT_USE_STD_PARALLEL
	#ifndef __cpp_lib_parallel_algorithm
		#define BDG_DONT_USE_STD_PARALLEL
	#endif
#endif // !BDG_DONT_USE_STD_PARALLE


#ifndef BDG_DONT_USE_STD_PARALLEL
	#include <execution>
	#include <mutex>
#endif //

// clang-format on

namespace fs = std::filesystem;

namespace mdev::boostdep {

namespace {

//################### Detect Modules #####################################

auto find_modules( fs::path const& path, const std::string& prefix = "" ) -> std::map<std::string, fs::path>
{
	std::map<std::string, fs::path> ret;

	for( const auto& entry : fs::directory_iterator( path ) ) {
		if( !entry.is_directory() ) {
			continue;
		}

		fs::path    mpath = entry.path();
		std::string mname = std::string( prefix + mpath.filename().string() );

		if( fs::exists( mpath / "sublibs" ) ) {
			auto r = find_modules( mpath, mname + "~" );
			ret.merge( r );
		}

		if( fs::exists( mpath / "include" ) ) {
			ret[mname] = mpath;
		}
	}
	return ret;
}

//################### Parse includes #####################################

std::string_view trim_left( std::string_view str )
{
	auto i = str.find_first_not_of( " \t" );
	if( i == std::string_view::npos ) {
		return std::string_view{};
	} else {
		return str.substr( i );
	}
}

std::string_view trim_left_with( std::string_view str, char c )
{
	auto i = str.find_first_not_of( " \t" );
	if( i == std::string_view::npos || str[i] != c ) {
		return std::string_view{};
	} else {
		return str.substr( i + 1 );
	}
}

// remove prefix from str
// returns view of remaining string, or empty string_view
std::string_view trim_prefix( std::string_view str, std::string_view prefix )
{
	if( str.substr( 0, prefix.size() ) != prefix ) {
		return std::string_view{};
	}
	return str.substr( prefix.size() );
}

std::string_view strip_quotes( std::string_view str )
{
	char terminator{};
	if( str[0] == '<' ) {
		terminator = '>';
	} else if( str[0] == '"' ) {
		terminator = '"';
	} else {
		return {}; // something strange happened
	}
	const auto k = str.find_first_of( terminator, 1 );
	if( k == std::string_view::npos ) {
		return {};
	}
	return str.substr( 1, k - 1 );
}

std::string_view get_included_file_from_line( std::string_view str )
{
	//                                   str=   #  include <boost/foo/bar/baz.h>
	str = trim_left_with( str, '#' ); // str=include <boost/foo/bar/baz.hpp>
	if( str.size() == 0 ) return {};
	str = trim_left( str );              // str=include <boost/foo/bar/baz.hpp>
	str = trim_prefix( str, "include" ); // str= <boost/foo/bar/baz.hpp>
	str = trim_left( str );              // str=<boost/foo/bar/baz.hpp>

	if( str.size() < 13 ) return {}; // this can't be a boost header if only 13 chars remain <boost/a.hpp>

	return strip_quotes( str ); // str=boost/foo/bar/baz.h
}

std::vector<std::string> get_included_boost_headers( fs::path const& file )
{
	std::vector<std::string> headers;
	std::ifstream            is( file );

	// Note: std::getline is the major performance bottleneck during scanning
	//
	// using fopen + fgets + fixed buffer, seems to reduce scan time by ~10-20%
	// on my windows machine (2.7s vs 3s)
	// I prefer the simpler c++ code for now
	for( std::string line; std::getline( is, line ); ) {
		if( line.size() < 20 ) {
			continue; // this can't be an include of a boost library
		}

		auto str = get_included_file_from_line( line );

		if( str == std::string_view{} || str.substr( 0, 6 ) != "boost/" ) {
			continue;
		}
		headers.emplace_back( str );
	}
	return headers;
}

/**
 * dir:  directory to search,
 * prefix: Filnames will be given relative to this director MUST BE A PARENT OF dir!
 */
std::vector<FileInfo>
scan_files_in_directory( fs::path const& dir, fs::path const& prefix, FileInfo base_template = {} )
{
	std::vector<FileInfo> discovered_files;
	if( !fs::exists( dir ) ) {
		return discovered_files;
	}

	const auto prefix_size = prefix.generic_string().size();
	for( auto& entry : fs::recursive_directory_iterator( dir ) ) {
		if( entry.is_regular_file() ) {
			FileInfo f = base_template;

			// fs::relative would be the "obvious" thing to do here, but it is much slower (at least on windows)
			f.name           = entry.path().generic_string().substr( prefix_size + 1 );
			f.included_files = get_included_boost_headers( entry.path() );

			discovered_files.push_back( std::move( f ) );
		}
	}
	return discovered_files;
}

std::vector<FileInfo> scan_module_files( const fs::path&   module_root,
										 const std::string module_name,
										 TrackSources      track_sources,
										 TrackTests        track_tests )
{
	FileInfo base_template;
	base_template.module_name = module_name;

	std::vector<FileInfo> ret;
	{
		base_template.category = FileCategory::Header;

		auto files = scan_files_in_directory( module_root / "include", module_root / "include", base_template );

		mdev::merge_into( std::move( files ), ret );
	}

	if( track_sources == TrackSources::Yes ) {

		base_template.category = FileCategory::Source;

		// filenames of source code file include module itself
		auto files = scan_files_in_directory( module_root / "src", module_root.parent_path(), base_template );

		mdev::merge_into( std::move( files ), ret );
	}

	if( track_tests == TrackTests::Yes ) {
		base_template.category = FileCategory::Test;

		auto files = scan_files_in_directory( module_root / "test", module_root.parent_path(), base_template );

		mdev::merge_into( std::move( files ), ret );
	}

	return ret;
}

} // namespace

std::vector<FileInfo>
scan_all_boost_modules( const fs::path& boost_root, const TrackSources track_sources, const TrackTests track_tests )
{
	const auto modules = find_modules( boost_root / "libs" );

	std::vector<FileInfo> module_infos;
	module_infos.reserve( modules.size() );
	// NOTE: In principle this is a classic map_reduce problem,
	// but my atempt in using std::transform_reduce ended in slower and more complicated code

#ifndef BDG_DONT_USE_STD_PARALLEL
	std::mutex mx;
#endif
	std::for_each( //
#ifndef BDG_DONT_USE_STD_PARALLEL
		std::execution::par, //
#endif
		modules.begin(), //
		modules.end(),   //
		[&]( const auto& m ) {
			auto ret = scan_module_files( m.second, m.first, track_sources, track_tests );
#ifndef BDG_DONT_USE_STD_PARALLEL
			std::lock_guard lg( mx );
#endif
			merge_into( std::move( ret ), module_infos );
		} //
	);
	std::sort( module_infos.begin(), //
			   module_infos.end(),   //
			   []( const auto& l, const auto& r ) { return l.module_name < r.module_name; } );
	return module_infos;
}

//########################################## analysis ########################################################

namespace {
std::map<std::string, FileInfo> make_file_map( std::vector<FileInfo> files )
{
	std::map<std::string, FileInfo> file_map;
	for( auto&& f : files ) {
		file_map.emplace( f.name, std::move( f ) );
	}
	return file_map;
}

auto filter_file_map( const std::map<std::string, FileInfo>& file_map, std::string root_module )
	-> std::map<std::string, FileInfo>
{
	// file_name -> file_info
	std::set<std::string> unprocessed_files;
	for( auto& [name, info] : file_map ) {
		if( info.module_name == root_module ) {
			unprocessed_files.insert( name );
		}
	}

	// file_name -> file_info
	std::map<std::string, FileInfo> filtered_file_map;
	while( unprocessed_files.size() != 0 ) {

		std::set<std::string> new_unprocessed_files;
		for( const auto& file : unprocessed_files ) {
			if( filtered_file_map.count( file ) == 0 ) {
				const auto& inc = file_map.at( file );

				filtered_file_map[file] = inc;
				new_unprocessed_files.insert( inc.included_files.begin(), inc.included_files.end() );
			}
		}
		std::swap( new_unprocessed_files, unprocessed_files );
	}

	return filtered_file_map;
}

auto file_map_to_module_dep_map( const std::map<std::string, FileInfo>& files )
	-> std::map<std::string, std::set<std::string>>
{
	std::map<std::string, std::set<std::string>> module_dependencies;
	for( auto& [name, info] : files ) {
		auto& m = module_dependencies[info.module_name];
		for( auto& d : info.included_files ) {
			try {
				m.insert( files.at( d ).module_name );
			} catch( const std::exception& ) {
				std::cout << "unknown file  included from " << name << " \t: " << d << std::endl;
			}
		}
	}
	return module_dependencies;
}

// tanslate internal format into the API format
DependencyInfo to_default_fomrat( std::map<std::string, std::set<std::string>>&& in )
{
	DependencyInfo ret;
	for( auto& m : in ) {
		m.second.erase( m.first ); // make sure there is no self-dependency
		ret[m.first] = mdev::to_vector<std::string>( std::move( m.second ) );
	}
	return ret;
}

DependencyInfo to_default_fomrat( std::map<std::string, FileInfo>&& in )
{
	DependencyInfo ret;
	for( auto& e : in ) {
		ret[e.first] = std::move( e.second.included_files );
	}
	return ret;
}

} // namespace

DependencyInfo build_module_dependency_map( const std::vector<FileInfo>& files )
{
	// file -> FileInfo
	const auto file_map = make_file_map( files );

	auto module_dependencies = file_map_to_module_dep_map( file_map );

	return to_default_fomrat( std::move( module_dependencies ) );
}

DependencyInfo build_filtered_module_dependency_map( const std::vector<FileInfo>& files, std::string root_module )
{
	const auto filtered_map = filter_file_map( make_file_map( files ), root_module );

	auto module_dependencies = file_map_to_module_dep_map( filtered_map );

	return to_default_fomrat( std::move( module_dependencies ) );
}

DependencyInfo build_filtered_file_dependency_map( const std::vector<FileInfo>& files, std::string root_module )
{
	auto filtered_map = filter_file_map( make_file_map( files ), root_module );

	// Add a fake file representing the root module
	auto& root_node       = filtered_map[root_module];
	root_node.name        = root_module;
	root_node.module_name = root_module;
	root_node.category    = FileCategory::Unknown;

	for( auto& file : files ) {
		if( file.module_name == root_module ) {
			root_node.included_files.push_back( file.name );
		}
	}

	std::sort( root_node.included_files.begin(), root_node.included_files.end() );

	return to_default_fomrat( std::move( filtered_map ) );
}

} // namespace mdev::boostdep
