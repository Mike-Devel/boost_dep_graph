#pragma once

#include <QFont>
#include <QSize>

namespace mdev::bdg::gui::cfg {

// constexpr double arrow_size  = 10;
// constexpr double node_radius = 10;
//
// constexpr double min_node_dist = 10; // ~The minimum distance between two nodes we try to hold
//
// const QFont         module_name_font = QFont( "Helvetica", 15 );
// constexpr QMarginsF margins( 10, 50, 60, 10 );
// constexpr QSizeF    scene_size = QSizeF{1800, 1100};

constexpr double arrow_size  = 10;
constexpr double node_radius = 10;

constexpr double min_node_dist = 13; // ~The minimum distance between two nodes we try to hold

const QFont module_name_font = [] {
	QFont f( "Helvetica", 14 );
	f.setBold( true );
	return f;
}();

constexpr QMarginsF margins( 10, 50, 60, 10 );
constexpr QSizeF    scene_size = QSizeF{2500, 1600};

} // namespace mdev::bdg::gui::cfg