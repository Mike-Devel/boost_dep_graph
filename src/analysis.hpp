#pragma once

#include "ModuleInfo.hpp"

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mdev::bdg {

modules_data generate_module_list( std::filesystem::path boost_root, const std::vector<std::string>& exclude = {} );

void set_direct_deps( modules_data& modules, std::map<std::string, std::set<std::string>> deps );

void update_derived_information( modules_data& modules );

std::vector<const ModuleInfo*> get_modules_sorted_by_dep_count( const modules_data& modules );

// update partial information
void update_cmake_status( ModuleInfo& module );

} // namespace mdev::bdg