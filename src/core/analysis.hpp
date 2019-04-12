#pragma once

#include "ModuleInfo.hpp"
#include "boostdep.hpp"

#include "utils.hpp"

#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mdev::bdg {

modules_data generate_file_list( const std::vector<boostdep::FileInfo>& files,
								 std::filesystem::path                  boost_root,
								 std::string                            root_module,
								 const std::vector<std::string>&        exclude = {} );

modules_data generate_module_list( const std::vector<boostdep::FileInfo>& files,
								   std::filesystem::path                  boost_root,
								   std::string                            root_module,
								   const std::vector<std::string>&        exclude = {} );

modules_data generate_module_list( const std::vector<boostdep::FileInfo>& files,
								   std::filesystem::path                  boost_root,
								   const std::vector<std::string>&        exclude = {} );

void update_derived_information( modules_data& modules );

std::vector<const ModuleInfo*> get_modules_sorted_by_dep_count( const modules_data& modules );

void update_cmake_status( ModuleInfo& module );

int block_count( const ModuleInfo& module );

void                                  print_cmake_stats( const modules_data& modules );
std::vector<std::vector<std::string>> cycles( const modules_data& modules );
modules_data                          subgraph( const modules_data& full_graph, span<const std::string> modules );

} // namespace mdev::bdg