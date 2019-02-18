#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace mdev::boostdep {

enum class TrackSources { No, Yes };
enum class TrackTests { No, Yes };
enum class TrackOrigin { No, Yes };

auto build_module_dependency_map( const std::filesystem::path& boost_root,
								  TrackSources                 track_sources,
								  TrackTests                   track_tests ) //
	-> std::map<std::string, std::set<std::string>>;

// This will only return modules that the files in root_module directly or indirectly depend on
// Note: This function is tracking actual include chains - not "library level dependencies"
auto build_filtered_module_dependency_map( const std::filesystem::path& boost_root,
										   std::string                  root_module,
										   TrackSources                 track_sources,
										   TrackTests                   track_tests ) //
	-> std::map<std::string, std::set<std::string>>;

auto build_filtered_file_dependency_map( const std::filesystem::path& boost_root,
										 std::string                  root_module,
										 TrackSources                 track_sources,
										 TrackTests                   track_tests ) //
	-> std::map<std::string, std::vector<std::string>>;
} // namespace mdev::boostdep
