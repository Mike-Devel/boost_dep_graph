#pragma once

#include <core/ModuleInfo.hpp>
#include <core/utils.hpp>

#include <QPoint>
#include <QRect>

namespace mdev::bdg::gui {

using ModuleLayout = std::map<String_t, QPoint>;

inline ModuleLayout layout_boost_modules( modules_data& modules, const QRectF area )
{
	// group modules by level and sort them such that in every level the modul with the most connections is in the
	// middle
	std::vector<std::vector<ModuleInfo*>> levels;

	for( auto&& [name, info] : modules ) {
		assert( info.level >= 0 );
		if( levels.size() <= static_cast<std::size_t>( info.level ) ) {
			levels.resize( info.level + 1 );
		}
		levels[info.level].push_back( &info );
	}

	auto cmp_by_dep_cnt = []( const ModuleInfo* l, const ModuleInfo* r ) {
		return l->rev_deps.size() * 5 + l->deps.size() < r->rev_deps.size() * 5 + r->deps.size();
	};

	for( auto& level : levels ) {
		sort_to_the_middle( level, cmp_by_dep_cnt );
	}

	// Translate the module order into x,y cooridnates
	ModuleLayout ret;

	int       xpos = area.x();
	const int dx   = area.width() / levels.size();

	int lvl = 1;
	for( auto& group : levels ) {

		// determine parameters for iteration (inital position  and position
		int ypos = 0;
		int dy   = 0;

		if( group.size() > 1 ) {
			ypos = area.y();
			dy   = ( area.height() - 10 ) / ( group.size() - 1 );
		} else {
			ypos = area.y() + area.height() / 2;
		}

		for( auto& module : group ) {

			ret[module->name] = QPoint( xpos, ypos );

			ypos += dy;
		}

		if( group.size() != 0 ) {

			auto& first = ret[group.front()->name];
			auto& last  = ret[group.back()->name];

			if( group.front()->rev_deps.size() > 0 ) {
				first.setY( first.y() + 10 );
			}

			if( group.back()->rev_deps.size() > 0 ) {
				last.setY( last.y() - 10 );
			}
		}
		xpos += dx;
		lvl++;
	}

	// manual adjustments
	ret["histogram"].setY( area.y() + area.height() / 3 );

	return ret;
}

} // namespace mdev::bdg::gui