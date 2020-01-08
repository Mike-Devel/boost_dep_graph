#pragma once

#include "utils.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace mdev::bdg {

struct ModuleInfo {

	String_t              name;
	bool                  has_cmake = false;
	std::set<ModuleInfo*> deps;
	std::set<ModuleInfo*> all_deps;

	// cached information
	int                   level = -1;
	bool                  deps_have_cmake = false;
	std::set<ModuleInfo*> rev_deps;
	std::set<ModuleInfo*> all_rev_deps;

	friend bool operator<( const ModuleInfo& l, const ModuleInfo& r ) { return l.name < r.name; }
};

struct modules_data : std::map<String_t, ModuleInfo> {
};

} // namespace mdev::bdg
