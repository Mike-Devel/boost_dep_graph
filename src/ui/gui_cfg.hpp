#pragma once

#include <QFont>
#include <QSize>

namespace mdev::bdg::gui::cfg {

constexpr double arrow_size  = 10;
constexpr double node_radius = 10;

constexpr double min_node_dist = 15; // ~The minimum distance between two nodes we try to hold

const QFont         module_name_font = QFont( "Helvetica", 15 );
constexpr QMarginsF margins( 20, 20, 80, 20 );
constexpr QSizeF    scene_size = QSizeF{1800, 1000};

} // namespace mdev::bdg::gui::cfg