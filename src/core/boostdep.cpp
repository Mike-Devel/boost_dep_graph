#include "boostdep.hpp"

#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <set>
#include <vector>

#include <iostream>

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

enum class FileCategory { Unknown, Header, Source, Test };

struct FileInfo {
	std::string              name;
	std::vector<std::string> included_files;
	std::string              module_name;
	FileCategory             category;
};

std::vector<FileInfo> scan_directory_for_boost_includes( fs::path const& dir )
{
	std::vector<FileInfo> included_headers;
	if( !fs::exists( dir ) ) {
		return included_headers;
	}

	const auto prefix_size = dir.generic_string().size();
	for( auto& entry : fs::recursive_directory_iterator( dir ) ) {
		if( entry.is_regular_file() ) {
			// fs::relative would be the "obvious" thing to do here, but it is much slower (at least on windows)
			auto file_name = entry.path().generic_string().substr( prefix_size + 1 );
			auto includes  = get_included_boost_headers( entry.path() );

			included_headers.emplace_back( FileInfo {std::move( file_name ), std::move( includes ), {}, {}} );
		}
	}
	return included_headers;
}

struct ModuleInfo {
	std::string           module_name;
	std::vector<FileInfo> files;
};

ModuleInfo scan_module_files( const fs::path&   module_root,
							  const std::string module_name,
							  TrackSources      track_sources,
							  TrackTests        track_tests )
{
	ModuleInfo ret;
	ret.module_name = module_name;

	{
		auto files = scan_directory_for_boost_includes( module_root / "include" );
		for( auto& f : files ) {
			f.category    = FileCategory::Header;
			f.module_name = module_name;
		}
		mdev::merge_into( std::move( files ), ret.files );
	}

	if( track_sources == TrackSources::Yes ) {
		auto files = scan_directory_for_boost_includes( module_root / "src" );

		const std::string prefix = ( module_root.filename() / "src" ).generic_string();
		for( auto& e : files ) {
			e.name        = prefix + e.name;
			e.category    = FileCategory::Source;
			e.module_name = module_name;
		}
		mdev::merge_into( std::move( files ), ret.files );
	}

	if( track_tests == TrackTests::Yes ) {
		auto files = scan_directory_for_boost_includes( module_root / "test" );

		const std::string prefix = ( module_root.filename() / "test" ).generic_string();
		for( auto& e : files ) {
			e.name        = prefix + e.name;
			e.category    = FileCategory::Test;
			e.module_name = module_name;
		}
	}

	return ret;
}

std::vector<ModuleInfo>
scan_all_boost_modules( const fs::path& boost_root, const TrackSources track_sources, const TrackTests track_tests )
{
	std::vector<ModuleInfo> module_infos;

#if BDG_DONT_USE_STD_PARALLEL
	for( auto&& [name, path] : find_modules( boost_root / "libs" ) ) {
		module_infos.emplace_back( scan_module_files( path, name, track_sources, track_tests ) );
	}
#else
	const auto modules = find_modules( boost_root / "libs" );
	module_infos.reserve( modules.size() );

	// NOTE: In principle this is a classic map_reduce problem,
	// but my atempt in using std::transform_reduce ended in slower and more complicated code
	std::mutex mx;
	std::for_each(           //
		std::execution::par, //
		modules.begin(),     //
		modules.end(),       //
		[&]( const auto& m ) {
			auto            ret = scan_module_files( m.second, m.first, track_sources, track_tests );
			std::lock_guard lg( mx );
			module_infos.emplace_back( std::move( ret ) );
		} //
	);
#endif
	std::sort( module_infos.begin(), //
			   module_infos.end(),   //
			   []( const auto& l, const auto& r ) { return l.module_name < r.module_name; } );
	return module_infos;
}

std::vector<FileInfo> aggregate_file_infos( const std::vector<ModuleInfo>& modules )
{
	std::vector<FileInfo> ret;
	for( const auto& m : modules ) {
		ret.insert( ret.end(), m.files.begin(), m.files.end() );
	}
	return ret;
}

std::map<std::string, FileInfo> make_file_map( const std::vector<FileInfo>& files )
{
	std::map<std::string, FileInfo> file_map;
	for( auto&& f : files ) {
		file_map[f.name] = f;
	}
	return file_map;
}

auto build_file_dependency_map( std::vector<FileInfo> files ) -> std::map<std::string, FileInfo>
{
	std::map<std::string, FileInfo> file_dep_map;
	for( auto&& f : files ) {
		file_dep_map.emplace( f.name, std::move( f ) );
	}
	return file_dep_map;
}

auto build_filtered_file_dependency_map( const std::vector<FileInfo>& files, std::string root_module )
	-> std::map<std::string, FileInfo>
{
	// file->includes
	std::map<std::string, FileInfo> complete_file_dep_map = build_file_dependency_map( files );

	std::vector<std::string> unprocessed_files;
	for( auto& fi : files ) {
		if( fi.module_name == root_module ) {
			unprocessed_files.push_back( fi.name );
		}
	}

	// file->includes
	std::map<std::string, FileInfo> filtered_file_map;
	while( unprocessed_files.size() != 0 ) {
		std::vector<std::string> tmp;

		for( auto& file : unprocessed_files ) {
			if( filtered_file_map.count( file ) == 0 ) {
				const auto& inc = complete_file_dep_map[file];

				filtered_file_map[file] = inc;
				tmp.insert( tmp.end(), inc.included_files.begin(), inc.included_files.end() );
			}
		}

		std::swap( tmp, unprocessed_files );
	}

	return filtered_file_map;
}

} // namespace

void updateInfo( const std::map<std::string, std::string> updates, std::vector<ModuleInfo>& modules )
{
	std::map<std::string, ModuleInfo> new_mods;
	for( auto&& u : updates ) {
		new_mods[u.second] = ModuleInfo {u.second};
	}

	for( auto& m : modules ) {
		bool found_some = false;
		for( auto f : m.files ) {
			auto it = updates.find( f.name );
			if( it != updates.end() ) {
				if( it->second != m.module_name ) {
					f.module_name = it->second;
					new_mods[it->second].files.push_back( f );
					found_some = true;
				}
			}
		}
		if( found_some ) {
			for( auto&& u : updates ) {
				m.files.erase( std::remove_if( m.files.begin(),
											   m.files.end(),
											   [&u]( const FileInfo& i ) { return i.name == u.first; } ),
							   m.files.end() );
			}
		}
	}

	for( auto&& n : new_mods ) {
		modules.push_back( n.second );
	}
}

const std::map<std::string, std::string> manual_assignments {
	{"boost/date_time/posix_time/time_serialize.hpp", "date_time~serialize"},
	{"boost/date_time/gregorian/greg_serialize.hpp", "date_time~serialize"},
	{"boost/dynamic_bitset/serialization.hpp", "dynamic_bitset~serialize"},
	{"boost/flyweight/serialize.hpp", "flyweight~serialize"},
	{"boost/flyweight/detail/archive_constructed.hpp", "flyweight~serialize"},
	{"boost/flyweight/detail/serialization_helper.hpp", "flyweight~serialize"},
	{"boost/units/io.hpp", "units~io"},
	{"boost/uuid/uuid_serialize.hpp", "uuid~serialize"}};

DependencyGraph to_default_fomrat( std::map<std::string, std::set<std::string>>&& in )
{
	DependencyGraph ret;
	for( auto& m : in ) {
		m.second.erase( m.first ); // make sure there is no self-dependency
		ret[m.first] = mdev::to_vector<std::string>( std::move( m.second ) );
	}
	return ret;
}

DependencyGraph to_default_fomrat( std::map<std::string, FileInfo>&& in )
{
	DependencyGraph ret;
	for( auto& e : in ) {
		ret[e.first] = std::move( e.second.included_files );
	}
	return ret;
}

DependencyGraph
build_module_dependency_map( const fs::path& boost_root, TrackSources track_sources, TrackTests track_tests )
{
	auto data = scan_all_boost_modules( boost_root, track_sources, track_tests );

	updateInfo( manual_assignments, data );

	const auto files      = aggregate_file_infos( data );
	const auto header_map = make_file_map( files );

	std::map<std::string, std::set<std::string>> module_dependencies;
	for( auto& f : files ) {
		auto& m = module_dependencies[f.module_name];
		for( auto& d : f.included_files ) {
			try {
				m.insert( header_map.at( d ).module_name );
			} catch( const std::exception& ) {
				std::cout << "unknown file  included from " << f.name << " \t: " << d << std::endl;
			}
		}
	}

	return to_default_fomrat( std::move( module_dependencies ) );
}

DependencyGraph build_filtered_module_dependency_map( const fs::path& boost_root,
										   std::string     root_module,
										   TrackSources    track_sources,
										   TrackTests track_tests )
{
	// Scan module files
	const auto modules = scan_all_boost_modules( boost_root, track_sources, track_tests );

	auto file_map = build_filtered_file_dependency_map( aggregate_file_infos( modules ), root_module );

	// Translate from file dependencies to module dependencies
	std::map<std::string, std::set<std::string>> module_map;
	for( auto& f : file_map ) {
		auto& m = module_map[f.second.module_name];
		for( auto& d : f.second.included_files ) {
			try {
				m.insert( file_map.at( d ).module_name );
			} catch( const std::exception& ) {
				std::cout << "unknown file  included from " << f.first << " \t: " << d << std::endl;
			}
		}
	}

	return to_default_fomrat( std::move( module_map ) );
}

DependencyGraph build_filtered_file_dependency_map( const std::filesystem::path& boost_root,
													std::string                  root_module,
													TrackSources                 track_sources,
													TrackTests                   track_tests )
{
	// Scan module files
	const auto modules = scan_all_boost_modules( boost_root, track_sources, track_tests );

	const auto root_module_it
		= std::find_if( modules.begin(), modules.end(), [&]( auto&& m ) { return m.module_name == root_module; } );
	if( root_module_it == modules.end() ) {
		std::cerr << "Error, specified root module " << root_module << " not found " << std::endl;
		return {};
	}

	auto map = build_filtered_file_dependency_map( aggregate_file_infos( modules ), root_module );

	// Add a fake file representing the root module
	auto& root_node       = map[root_module];
	root_node.name        = root_module;
	root_node.module_name = root_module;
	root_node.category    = FileCategory::Unknown;

	for( auto& file : root_module_it->files ) {
		root_node.included_files.push_back( file.name );
	}

	return to_default_fomrat( std::move( map ) );
}



} // namespace mdev::boostdep
