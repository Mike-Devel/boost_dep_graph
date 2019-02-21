#include "edges.hpp"

#include "gui_cfg.hpp"
#include "node.hpp"

#include <core/ModuleInfo.hpp>

#include <QGraphicsScene>
#include <QPainter>

#include <cmath>

namespace mdev::bdg::gui {

namespace {

double calc_diff( QLineF l, QLineF r )
{
	double dx1 = l.p1().x() - r.p1().x();
	double dy1 = l.p1().y() - r.p1().y();

	double dx2 = l.p2().x() - r.p2().x();
	double dy2 = l.p2().y() - r.p2().y();

	return dx1 * dx1 + dy1 * dy1 + dx2 * dx2 + dy2 * dy2;
}

QPolygonF get_arrow_head( QLineF line )
{
	constexpr double half_width_angle = 3.1416 / 3 / 2;

	double angle = std::atan2( line.dy(), line.dx() );

	QPointF t1 = line.p2()
				 - QPointF( cos( angle - half_width_angle ) * cfg::arrow_size,
							sin( angle - half_width_angle ) * cfg::arrow_size );
	QPointF t2 = line.p2()
				 - QPointF( cos( angle + half_width_angle ) * cfg::arrow_size,
							sin( angle + half_width_angle ) * cfg::arrow_size );

	return QPolygonF( {line.p2(), t1, t2} );
}

QLineF get_arrow_line( QLineF endpoints )
{
	auto length = endpoints.length();

	QPointF edgeOffset( ( endpoints.dx() * cfg::node_radius ) / length,
						( endpoints.dy() * cfg::node_radius ) / length );

	return QLineF{endpoints.p1() + edgeOffset, endpoints.p2() - edgeOffset};
}

} // namespace

namespace detail {

Edge::Edge()
{
	setAcceptedMouseButtons( 0 );
}

Edge::Edge( Node* src, Node* dst )
	: _src( src )
	, _dest( dst )
{
	setAcceptedMouseButtons( 0 );
}

void Edge::update_positions()
{
	if( !_src || !_dest ) return;

	QLineF node_positions( _src->pos(), _dest->pos() );

	if( calc_diff( node_positions, _cached_endpoints ) <= 5.0 ) {
		return;
	}

	prepareGeometryChange();

	_cached_endpoints = node_positions;

	if( node_positions.length() > cfg::node_radius ) {
		_cached_arrow_line = get_arrow_line( node_positions );
		_cached_arrow_head = get_arrow_head( _cached_arrow_line );
	} else {
		_cached_arrow_head.clear();
		_cached_arrow_line = QLineF{};
	}
}

void Edge::update_style()
{
	if( !_src || !_dest ) return;

	auto is_selected = _src->is_selected() || _dest->is_selected();
	auto no_cmake    = !_dest->info()->has_cmake;

	QColor line_color = no_cmake ? Qt::red : ( is_selected ? Qt::black : Qt::darkGray );

	int line_width = is_selected ? 3 : 1;

	if( is_selected ) {
		this->setZValue( 1 );
	} else if( no_cmake ) {
		this->setZValue( -1 );
	} else {
		this->setZValue( -2 );
	}

	_line_pen = QPen( line_color, line_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
}

QRectF Edge::boundingRect() const
{
	qreal extra = ( _line_pen.width() + cfg::arrow_size ) / 2.0;

	return QRectF( _cached_endpoints.p1(), QSizeF( _cached_endpoints.dx(), _cached_endpoints.dy() ) )
		.normalized()
		.adjusted( -extra, -extra, extra, extra );
}

void Edge::paint( QPainter* painter, const QStyleOptionGraphicsItem*, QWidget* )
{
	if( _cached_arrow_line.isNull() ) {
		return;
	}

	painter->setPen( _line_pen );
	painter->setBrush( _line_pen.color() );

	painter->drawLine( _cached_arrow_line );
	painter->drawPolygon( _cached_arrow_head );
}

} // namespace detail

void Edges::clear()
{
	_edge_list.clear();
}

void Edges::update_positions()
{
	for( auto& e : _edge_list ) {
		e.update_positions();
	}
}

void Edges::update_style()
{
	for( auto& e : _edge_list ) {
		e.update_style();
	}
}

void Edges::update_all()
{
	update_style();
	update_positions();
}

void Edges::create_edges( QGraphicsScene& scene, mdev::span<const std::pair<Node*, Node*>> connections )
{
	clear();
	// The Edge class is non_copyable so we have to jump through some hoops to use it in a vector
	_edge_list = std::vector<detail::Edge>( connections.size() );
	int i      = 0;
	for( const auto& [src, dest] : connections ) {
		_edge_list[i].set_nodes( src, dest );
		i++;
	}
	for( detail::Edge& e : _edge_list ) {
		scene.addItem( &e );
	}
	update_all();
}

} // namespace mdev::bdg::gui
