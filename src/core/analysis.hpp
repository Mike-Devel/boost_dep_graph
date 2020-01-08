#pragma once

#include "ModuleInfo.hpp"
#include "boostdep.hpp"

#include "utils.hpp"

#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace mdev::bdg {

modules_data generate_file_list( const std::vector<boostdep::FileInfo>& files,
								 std::filesystem::path                  boost_root,
								 String_t                               root_module,
								 const std::vector<String_t>&           exclude = {} );

modules_data generate_module_list( const std::vector<boostdep::FileInfo>& files,
								   std::filesystem::path                  boost_root,
								   const std::optional<String_t>&         root_module,
								   const std::vector<String_t>&           exclude = {} );

void update_derived_information( modules_data& modules );

std::vector<const ModuleInfo*> get_modules_sorted_by_dep_count( const modules_data& modules );

void update_cmake_status( ModuleInfo& module );

int block_count( const ModuleInfo& module );

void print_cmake_stats( const modules_data& modules );
auto cycles( const modules_data& modules ) -> std::vector<std::vector<String_t>>;
auto subgraph( const modules_data& full_graph, span<const String_t> modules ) -> modules_data;

} // namespace mdev::bdg