#include <algorithm>
#include <cassert>
#include <climits>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>

namespace fs = std::filesystem;

namespace boostdep {

//################### Detect headers #####################################
namespace {

auto find_modules( fs::path const& path, const std::string& prefix = "" ) -> std::map<std::string,fs::path>
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

auto discover_headers( fs::path const& root_dir ) -> std::vector<std::string>
{
	std::vector<std::string> headers;

	for( auto&& entry : fs::recursive_directory_iterator( root_dir ) ) {
		if( !entry.is_directory() ) {
			auto extension = entry.path().extension();
			if( extension == ".hpp" || extension == ".h" ) {
				headers.push_back( fs::relative( entry.path(), root_dir ).generic_string() );
			}
		}
	}
	return headers;
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

// remove prefix <pattern> from str
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
	// line=   #  include <boost/foo/bar/baz.h>
	str = trim_left( str );              // str=#  include <boost/foo/bar/baz.h>
	str = trim_prefix( str, "#" );       // str=  include <boost/foo/bar/baz.h>
	str = trim_left( str );              // str=include <boost/foo/bar/baz.h>
	str = trim_prefix( str, "include" ); // str= <boost/foo/bar/baz.h>
	str = trim_left( str );              // str=<boost/foo/bar/baz.h>

	if( str.size() < 12 ) return {}; // this can't be a boost header if only 10 chars remain

	return strip_quotes( str ); // str=boost/foo/bar/baz.h
}

std::set<std::string> get_included_boost_headers( fs::path const& file )
{
	std::set<std::string> headers;
	std::ifstream         is( file );
	for( std::string line; std::getline( is, line ); ) {
		if( line.size() < 20 ) {
			continue; // this can't be an include of a boost library
		}

		auto str = get_included_file_from_line( line );

		if( str == std::string_view{} || str.substr( 0, 6 ) != "boost/" ) {
			continue;
		}
		headers.emplace( str );
	}
	return headers;
}

std::set<std::string> scan_directory_for_boost_includes( fs::path const& dir )
{
	std::set<std::string> included_headers;
	if( !fs::exists( dir ) ) {
		return included_headers;
	}
	for( auto& entry : fs::recursive_directory_iterator( dir ) ) {
		if( entry.is_regular_file() ) {
			auto includs = get_included_boost_headers( entry.path() );
			included_headers.merge( includs );
		}
	}
	return included_headers;
}


struct ModuleInfo {
	std::vector<std::string> headers;
	std::set<std::string>    includes;
};

ModuleInfo scan_module_files( const fs::path& module_root,
							  bool                         track_sources,
							  bool                         track_tests )
{
	ModuleInfo ret;
	ret.headers = discover_headers( module_root / "include" );
	{
		auto s = scan_directory_for_boost_includes( module_root / "include" );
		ret.includes.merge( s );
	}

	if( track_sources ) {
		auto s = scan_directory_for_boost_includes( module_root / "src" );
		ret.includes.merge( s );
	}

	if( track_tests ) {
		auto s = scan_directory_for_boost_includes( module_root / "test" );
		ret.includes.merge( s );
	}

	return ret;
}

} // namespace

using namespace std::chrono_literals;

auto build_module_dependency_map( const fs::path& boost_root, bool track_sources, bool track_tests )
	-> std::map<std::string, std::set<std::string>>
{
	//auto start = std::chrono::system_clock::now();
	// Scan module files
	std::map<std::string, ModuleInfo>  modules;
	for( auto&& [name,path] : find_modules( boost_root / "libs" ) ) {
		modules[name] = scan_module_files( path, track_sources, track_tests );
	}
	//auto end = std::chrono::system_clock::now();
	//std::cout << ( end - start ) / 1ms << std::endl;

	// create header map
	std::map<std::string, std::string> header_map;
	for( auto&& [name,info] : modules ) {
		for( auto&& h : info.headers ) {
			header_map[h] = name;
		}
	}

	// Create dependency map
	std::map<std::string, std::set<std::string>> module_dependencies;
	for( auto&[module,info] : modules ) {
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

} // namespace boostdep
