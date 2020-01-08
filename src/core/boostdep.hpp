#pragma once

#include "utils.hpp"

#include <filesystem>
#include <map>
#include <string>

namespace mdev::boostdep {

enum class TrackSources { No, Yes };
enum class TrackTests { No, Yes };
enum class TrackOrigin { No, Yes };

enum class FileCategory { Unknown, Header, Source, Test };
struct FileInfo {
	String_t              name;
	std::vector<String_t> included_files;
	String_t              module_name;
	FileCategory          category;
};

std::vector<FileInfo> scan_all_boost_modules( const std::filesystem::path& boost_root,
											  const TrackSources           track_sources,
											  const TrackTests             track_tests );

using DependencyInfo = std::map < String_t, std::vector<String_t>> ;

DependencyInfo build_module_dependency_map( const std::vector<FileInfo>& files );

// This will only return modules that the files in root_module directly or indirectly depend on
// Note: This function is tracking actual include chains - not "library level dependencies"
DependencyInfo build_filtered_module_dependency_map( const std::vector<FileInfo>& files, std::string_view root_module );

DependencyInfo build_filtered_file_dependency_map( const std::vector<FileInfo>& files, std::string_view root_module );
} // namespace mdev::boostdep
