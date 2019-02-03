#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace boostdep {

std::map<std::string, std::set<std::string>> build_module_dependency_map( const std::filesystem::path& boost_root, bool track_sources, bool track_tests );

} // namespace boost
