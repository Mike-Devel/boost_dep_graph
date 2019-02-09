#pragma once

#include <QGraphicsItem>

#include <QColor>
#include <QLine>
#include <QPen>
#include <QPolygon>

#include "../utils.hpp"

#include <memory>

namespace mdev::bdg::gui {

class Node;

namespace detail {

class Edge : public QGraphicsItem {
public:
	Edge();
	Edge( Node* src, Node* dst );

	void update_positions();
	void update_style();

	void set_nodes( Node* src, Node* dest )
	{
		_src  = src;
		_dest = dest;
	}

protected:
	QRectF boundingRect() const;
	void   paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget );

private:
	Node* _src  = nullptr;
	Node* _dest = nullptr;

	// cached data for painting
	QPen      _line_pen {};
	QLineF    _cached_endpoints {};
	QLineF    _cached_arrow_line {};
	QPolygonF _cached_arrow_head {};
};

} // namespace detail

class Edges {
	std::vector<detail::Edge> _edge_list;

public:
	void create_edges( QGraphicsScene& scene, mdev::span<const std::pair<Node*, Node*>> connections );
	void clear();
	void update_positions();
	void update_style();
};

} // namespace mdev::bdg::gui
