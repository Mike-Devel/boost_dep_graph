#pragma once

#include "../ModuleInfo.hpp"
#include "../utils.hpp"

namespace mdev::bdg::gui {

using ModuleLayout = std::vector<std::vector<ModuleInfo*>>;

ModuleLayout layout_boost_modules( modules_data& modules )
{
	auto max_level = std::max_element( modules.begin(),
									   modules.end(),
									   []( auto& l, auto& r ) { return l.second.level < r.second.level; } )
						 ->second.level;

	ModuleLayout ret( max_level + 1 );

	for( auto&& [name, info] : modules ) {
		ret[info.level].push_back( &info );
	}

	auto cmp_by_dep_cnt = []( const ModuleInfo* l, const ModuleInfo* r ) {
		return l->rev_deps.size() * 5 + l->deps.size() < r->rev_deps.size() * 5 + r->deps.size();
	};

	for( auto& level : ret ) {
		sort_to_the_middle( level, cmp_by_dep_cnt );
	}

	return ret;
}

} // namespace mdev::bdg