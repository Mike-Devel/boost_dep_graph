#include "boostdep.hpp"

#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <vector>

// clang-format off
#ifndef BDG_DONT_USE_STD_PARALLEL
	#ifdef __cpp_lib_parallel_algorithm
		#define BDG_DONT_USE_STD_PARALLEL 0
	#else
		#define BDG_DONT_USE_STD_PARALLEL 1
	#endif
#endif // !BDG_DONT_USE_STD_PARALLE

#if !BDG_DONT_USE_STD_PARALLEL
	#include <execution>
	#include <mutex>
#endif //

// clang-format on

namespace fs = std::filesystem;

namespace mdev::boostdep {

namespace {

//################### Detect headers #####################################

auto find_modules( fs::path const& path, const std::string& prefix = "" ) -> std::map<std::string, fs::path>
{
	std::map<std::string, fs::path> ret;

	for( const auto& entry : fs::directory_iterator( path ) ) {
		if( !entry.is_directory() ) {
			continue;
		}

		fs::path    mpath       = entry.path();
		std::string module_name = std::string( prefix + mpath.filename().string() );

		if( fs::exists( mpath / "sublibs" ) ) {
			auto r = find_modules( mpath, module_name + "~" );
			ret.merge( r );
		}

		if( fs::exists( mpath / "include" ) ) {
			ret[module_name] = mpath;
		}
	}
	return ret;
}

//################### Parse includes #####################################

std::string_view trim_left( std::string_view str )
{
	auto i = str.find_first_not_of( " \t" );
	if( i == std::string_view::npos ) {
		return std::string_view {};
	} else {
		return str.substr( i );
	}
}

// remove prefix <pattern> from str
// returns view of remaining string, or empty string_view
std::string_view trim_prefix( std::string_view str, std::string_view prefix )
{
	if( str.substr( 0, prefix.size() ) != prefix ) {
		return std::string_view {};
	}
	return str.substr( prefix.size() );
}

std::string_view strip_quotes( std::string_view str )
{
	char terminator {};
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
	// line=   #  include <boost/foo/bar/baz.h>
	str = trim_left( str );        // str=#  include <boost/foo/bar/baz.hpp>
	str = trim_prefix( str, "#" ); // str=  include <boost/foo/bar/baz.hpp>
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

		if( str == std::string_view {} || str.substr( 0, 6 ) != "boost/" ) {
			continue;
		}
		headers.emplace_back( str );
	}
	return headers;
}

struct IncludeInfo {
	std::string              name;
	std::vector<std::string> included_files;
};

std::vector<IncludeInfo> scan_directory_for_boost_includes( fs::path const& dir )
{
	std::vector<IncludeInfo> included_headers;
	if( !fs::exists( dir ) ) {
		return included_headers;
	}

	const auto prefix_size = dir.generic_string().size();
	for( auto& entry : fs::recursive_directory_iterator( dir ) ) {
		if( entry.is_regular_file() ) {
			// fs::relative would be the "obvious" thing to do here, but it is much slower (at least on windows)
			auto file_name = entry.path().generic_string().substr( prefix_size + 1 );
			auto includes  = get_included_boost_headers( entry.path() );

			included_headers.emplace_back( IncludeInfo {std::move( file_name ), std::move( includes )} );
		}
	}
	return included_headers;
}

struct ModuleInfo {
	std::vector<IncludeInfo> header_includes;
	std::vector<IncludeInfo> source_includes;
	std::vector<IncludeInfo> test_includes;
};

ModuleInfo scan_module_files( const fs::path& module_root, TrackSources track_sources, TrackTests track_tests )
{
	ModuleInfo ret;
	ret.header_includes = scan_directory_for_boost_includes( module_root / "include" );

	if( track_sources == TrackSources::Yes ) {
		ret.source_includes = scan_directory_for_boost_includes( module_root / "src" );
		std::string prefix  = module_root.filename().string() + "src";
		for( auto& e : ret.source_includes ) {
			e.name = prefix + e.name;
		}
	}
	if( track_tests == TrackTests::Yes ) {
		ret.test_includes  = scan_directory_for_boost_includes( module_root / "test" );
		std::string prefix = module_root.filename().string() + "test";
		for( auto& e : ret.source_includes ) {
			e.name = prefix + e.name;
		}
	}
	return ret;
}

struct AggregatedModuleInfo {
	std::vector<std::string> headers;
	std::set<std::string>    includes;
};

template<TrackOrigin track_origin>
using ScanResult = std::conditional_t<track_origin == TrackOrigin::Yes, ModuleInfo, AggregatedModuleInfo>;

AggregatedModuleInfo aggregate( const ModuleInfo& in )
{
	std::vector<std::string> headers;
	headers.reserve( in.header_includes.size() );
	for( const auto& file : in.header_includes ) {
		headers.push_back( file.name );
	}
	std::sort( headers.begin(), headers.end() );

	std::set<std::string> includes;
	for( const auto& file : in.header_includes ) {
		for( const auto& i : file.included_files ) {
			includes.insert( i );
		}
	}
	for( const auto& file : in.source_includes ) {
		for( const auto& i : file.included_files ) {
			includes.insert( i );
		}
	}
	for( const auto& file : in.test_includes ) {
		for( const auto& i : file.included_files ) {
			includes.insert( i );
		}
	}

	return {std::move( headers ), std::move( includes )};
}

template<TrackOrigin track_origin>
std::map<std::string, ScanResult<track_origin>>
scan_all_boost_modules( const fs::path& boost_root, const TrackSources track_sources, const TrackTests track_tests )
{
	std::map<std::string, ScanResult<track_origin>> module_infos;

#if BDG_DONT_USE_STD_PARALLEL
	for( auto&& [name, path] : find_modules( boost_root / "libs" ) ) {
		module_infos[name] = scan_module_files<track_origin>( path, track_sources, track_tests );
	}
#else
	const auto modules = find_modules( boost_root / "libs" );

	// NOTE: In principle this is a classic map_reduce problem,
	// but my atempt in using std::transform_reduce ended in slower and more complicated code
	std::mutex mx;
	std::for_each(           //
		std::execution::par, //
		modules.begin(),     //
		modules.end(),       //
		[&]( const auto& m ) {
			auto            ret = scan_module_files( m.second, track_sources, track_tests );
			std::lock_guard lg( mx );
			if constexpr( track_origin == TrackOrigin::Yes ) {
				module_infos[m.first] = std::move( ret );
			} else {
				module_infos[m.first] = aggregate( ret );
			}
		} //
	);
#endif

	return module_infos;
}

std::map<std::string, std::string> make_header_map( const std::map<std::string, AggregatedModuleInfo>& modules )
{
	std::map<std::string, std::string> header_map;
	for( auto&& [name, info] : modules ) {
		for( auto&& h : info.headers ) {
			header_map[h] = name;
		}
	}
	return header_map;
}

std::map<std::string, std::string> make_file_map( const std::map<std::string, ModuleInfo>& modules )
{
	std::map<std::string, std::string> file_map;
	for( auto&& [name, info] : modules ) {
		for( auto&& h : info.header_includes ) {
			file_map[h.name] = name;
		}
		for( auto&& h : info.source_includes ) {
			file_map[h.name] = name;
		}
		for( auto&& h : info.test_includes ) {
			file_map[h.name] = name;
		}
	}
	return file_map;
}

auto build_filtered_file_dependency_map( const std::map<std::string, ModuleInfo>& modules, std::string root_module )
	-> std::map<std::string, std::vector<std::string>>
{
	// file->includes
	std::map<std::string, std::vector<std::string>> complete_file_dep_map;

	for( auto& module : modules ) {
		for( auto& file : module.second.header_includes ) {
			complete_file_dep_map.emplace( file.name, file.included_files );
		}
		for( auto& file : module.second.source_includes ) {
			complete_file_dep_map.emplace( file.name, file.included_files );
		}
		for( auto& file : module.second.test_includes ) {
			complete_file_dep_map.emplace( file.name, file.included_files );
		}
	}

	auto& root_module_e = modules.at( root_module );

	std::vector<std::string> unprocessed_files;
	for( auto& [file, includes] : root_module_e.header_includes ) {
		unprocessed_files.push_back( file );
	}
	for( auto& [file, includes] : root_module_e.source_includes ) {
		unprocessed_files.push_back( file );
	}
	for( auto& [file, includes] : root_module_e.test_includes ) {
		unprocessed_files.push_back( file );
	}

	// file->includes
	std::map<std::string, std::vector<std::string>> filtered_file_map;
	while( unprocessed_files.size() != 0 ) {
		std::vector<std::string> tmp;

		for( auto& file : unprocessed_files ) {
			if( filtered_file_map.count( file ) == 0 ) {
				const auto& inc = complete_file_dep_map[file];

				filtered_file_map[file] = inc;
				tmp.insert( tmp.end(), inc.begin(), inc.end() );
			}
		}

		std::swap( tmp, unprocessed_files );
	}

	return filtered_file_map;
}

} // namespace

auto build_module_dependency_map( const fs::path& boost_root, TrackSources track_sources, TrackTests track_tests )
	-> std::map<std::string, std::set<std::string>>
{
	std::map<std::string, AggregatedModuleInfo> modules
		= scan_all_boost_modules<TrackOrigin::No>( boost_root, track_sources, track_tests );

	auto header_map = make_header_map( modules );

	// Create dependency map
	std::map<std::string, std::set<std::string>> module_dependencies;
	for( auto& [module, info] : modules ) {
		auto& deps = module_dependencies[module];
		for( auto&& h : info.includes ) {
			const auto i = header_map.find( h );
			if( i != header_map.end() ) {
				deps.emplace( i->second );
			}
		}
		deps.erase( module ); // remove self dependency
	}

	return module_dependencies;
}

auto build_filtered_file_dependency_map( const std::filesystem::path& boost_root,
										 std::string                  root_module,
										 TrackSources                 track_sources,
										 TrackTests                   track_tests ) //
	-> std::map<std::string, std::vector<std::string>>
{
	// Scan module files
	const std::map<std::string, ModuleInfo> modules
		= scan_all_boost_modules<TrackOrigin::Yes>( boost_root, track_sources, track_tests );

	auto map = build_filtered_file_dependency_map( modules, root_module );

	auto& root_module_e = modules.at( root_module );
	auto& root_node     = map[root_module];

	for( auto& [file, includes] : root_module_e.header_includes ) {
		root_node.push_back( file );
	}
	for( auto& [file, includes] : root_module_e.source_includes ) {
		root_node.push_back( file );
	}
	for( auto& [file, includes] : root_module_e.test_includes ) {
		root_node.push_back( file );
	}

	return map;
}

namespace {
std::string to_module( const std::map<std::string, std::string>& map, const std::string& file )
{
	auto it = map.find( file );
	if( it != map.end() ) {
		return it->second;
	} else {
		return file;
	}
}
} // namespace

auto build_filtered_module_dependency_map( const fs::path& boost_root,
										   std::string     root_module,
										   TrackSources    track_sources,
										   TrackTests      track_tests ) -> std::map<std::string, std::set<std::string>>
{
	const std::map<std::string, ModuleInfo> modules
		= scan_all_boost_modules<TrackOrigin::Yes>( boost_root, track_sources, track_tests );

	const auto file_deps = build_filtered_file_dependency_map( modules, root_module );

	// file -> module
	const auto file_map = make_file_map( modules );

	std::map<std::string, std::set<std::string>> filtered_module_map;

	for( auto&& e : file_deps ) {
		auto& deps = filtered_module_map[to_module( file_map, e.first )];
		for( auto&& include : e.second ) {
			deps.insert( to_module( file_map, include ) );
		}
	}
	return filtered_module_map;
}

} // namespace mdev::boostdep
