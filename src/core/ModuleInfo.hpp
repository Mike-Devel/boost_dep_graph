#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace mdev::bdg {

struct ModuleInfo {

	std::string           name;
	int                   level;
	bool                  has_cmake;
	bool                  deps_have_cmake;
	std::set<ModuleInfo*> deps;
	std::set<ModuleInfo*> all_deps;
	std::set<ModuleInfo*> rev_deps;
	std::set<ModuleInfo*> all_rev_deps;

	friend bool operator<( const ModuleInfo& l, const ModuleInfo& r ) { return l.name < r.name; }
};

struct modules_data : public std::map<std::string, ModuleInfo> {
};

} // namespace mdev::bdg
