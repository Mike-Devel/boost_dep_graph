#include "analysis.hpp"

#include "boostdep.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>

namespace {

template<class T, class F>
T filter( const T& data, const F& exclude )
{
	T ret;
	for( const auto& e : data ) {
		if( std::count( exclude.begin(), exclude.end(), e ) == 0 ) {
			ret.push_back( e );
		}
	}
	return ret;
}

std::string replace( const std::string& src, char match, char replacement )
{
	std::string ret = src;
	std::replace( ret.begin(), ret.end(), match, replacement );
	return ret;
}

mdev::bdg::modules_data process_dpendency_map( const std::map<std::string, std::set<std::string>>& dependency_map,
											   std::filesystem::path                               boost_root,
											   const std::vector<std::string>&                     exclude )
{

	std::vector<std::string> module_names;
	for( auto [name, ignore] : dependency_map ) {
		module_names.push_back( name );
	}

	mdev::bdg::modules_data data;
	for( auto& name : filter( module_names, exclude ) ) {

		std::string relative_path_to_root = replace( name, '~', '/' );

		bool has_cmake = std::filesystem::exists( boost_root / "libs" / relative_path_to_root / "CMakeLists.txt" );

		data[name] = mdev::bdg::ModuleInfo {name, -1, has_cmake, false};
	}

	set_direct_deps( data, dependency_map );

	update_derived_information( data );

	return data;
}

} // namespace

namespace mdev::bdg {

modules_data
generate_module_list( std::filesystem::path boost_root, std::string root, const std::vector<std::string>& exclude )
{
	auto dependency_map = boostdep::build_filtered_module_dependency_map(
		boost_root, root, boostdep::TrackSources::Yes, boostdep::TrackTests::No );

	return process_dpendency_map( dependency_map, boost_root, exclude );

}

modules_data generate_module_list( std::filesystem::path boost_root, const std::vector<std::string>& exclude )
{

	auto dependency_map
		= boostdep::build_module_dependency_map( boost_root, boostdep::TrackSources::Yes, boostdep::TrackTests::No );

	return process_dpendency_map( dependency_map, boost_root, exclude );
}

modules_data
generate_file_list( std::filesystem::path boost_root, std::string root, const std::vector<std::string>& exclude )
{
	auto dependency_map = boostdep::build_filtered_file_dependency_map(
		boost_root, root, boostdep::TrackSources::Yes, boostdep::TrackTests::No );

	std::map<std::string, std::set<std::string>> dependency_map2;


	for( auto&& file : dependency_map ) {
		auto& e = dependency_map2[file.first];
		for( auto&& dep : file.second ) {
			e.insert( dep );
		}
	}

	return process_dpendency_map( dependency_map2, boost_root, exclude );
}

void set_direct_deps( modules_data& modules, std::map<std::string, std::set<std::string>> deps )
{
	for( auto& [name1, info1] : modules ) {
		for( auto& [name2, info2] : modules ) {
			if( deps[name1].count( name2 ) != 0 ) {
				info1.deps.insert( &info2 );
				info2.rev_deps.insert( &info1 );
			}
		}
	}
}

void update_transitive_dependencies( modules_data& modules )
{
	// initialize with direct dependencies
	for( auto& [ignore, info] : modules ) {
		info.all_deps     = info.deps;
		info.all_rev_deps = info.rev_deps;
	}

	// build transitive closure
	for( auto& [dont_care, module] : modules ) {
		for( auto prev_dep_cnt = (std::size_t)0; prev_dep_cnt != module.all_deps.size(); ) {

			prev_dep_cnt = module.all_deps.size();
			std::set<ModuleInfo*> new_deps;
			for( auto* d : module.all_deps ) {
				new_deps.insert( d->all_deps.begin(), d->all_deps.end() );
			}
			module.all_deps.merge( new_deps );
		}
	}

	// update reverse dependencies
	for( auto& [dont_care, info] : modules ) {
		for( auto* dep : info.all_deps ) {
			dep->all_rev_deps.insert( &info );
		}
	}
}

void update_module_levels( modules_data& modules )
{
	for( auto& m : modules ) {
		m.second.level = -1;
	}

	bool        updated = true;
	std::size_t cnt     = 0;
	while( updated && cnt++ < modules.size() + 1 ) {
		updated = false;
		for( auto& [module, info] : modules ) {

			int max_dep_level = -1;
			for( auto* d : info.all_deps ) {
				// only check libs that are not in a dependency cycle with us
				if( d->all_deps.count( &info ) == 0 ) {
					max_dep_level = std::max( max_dep_level, d->level );
				}
			}
			const int new_level = max_dep_level + 1;

			if( info.level != new_level ) {
				updated = true;
			}
			info.level = new_level;
		}
	}

	for( auto& m : modules ) {
		assert( m.second.level != -1 );
		(void)m;
	}
}

void update_cmake_status( ModuleInfo& info )
{
	info.deps_have_cmake = std::all_of( info.all_deps.begin(), //
										info.all_deps.end(),   //
										[]( auto i ) { return i->has_cmake; } );
}

void update_cmake_status( modules_data& modules )
{
	for( auto& [name, info] : modules ) {
		update_cmake_status( info );
	}
}

void update_derived_information( modules_data& modules )
{
	update_transitive_dependencies( modules );
	update_module_levels( modules );
	update_cmake_status( modules );
}

std::vector<const ModuleInfo*> get_modules_sorted_by_dep_count( const modules_data& modules )
{
	std::vector<const ModuleInfo*> list;
	list.reserve( modules.size() );
	for( auto&& m : modules ) {
		list.push_back( &m.second );
	}
	std::sort( list.begin(), list.end(), []( const ModuleInfo* l, auto r ) {
		return l->all_rev_deps.size() > r->all_rev_deps.size();
	} );

	return list;
}

} // namespace mdev::bdg
