#pragma once

#include "ModuleInfo.hpp"

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mdev::bdg {

modules_data generate_file_list( std::filesystem::path           boost_root,
									std::string                     root_module,
									const std::vector<std::string>& exclude = {} );

modules_data generate_module_list( std::filesystem::path           boost_root,
								   std::string                     root_module,
								   const std::vector<std::string>& exclude = {} );
modules_data generate_module_list( std::filesystem::path boost_root, const std::vector<std::string>& exclude = {} );

void set_direct_deps( modules_data& modules, std::map<std::string, std::set<std::string>> deps );

void update_derived_information( modules_data& modules );

std::vector<const ModuleInfo*> get_modules_sorted_by_dep_count( const modules_data& modules );

void update_cmake_status( ModuleInfo& module );

inline int block_count( const ModuleInfo& module )
{
	if( module.has_cmake ) {
		return 0;
	}
	int blocked_modules = 0;
	for( auto* m : module.all_rev_deps ) {
		const auto cnt
			= std::count_if( m->all_deps.begin(), m->all_deps.end(), []( ModuleInfo* m ) { return !m->has_cmake; } );
		if( cnt == 1 ) {
			blocked_modules++;
		}
	}
	return blocked_modules;
}

} // namespace mdev::bdg