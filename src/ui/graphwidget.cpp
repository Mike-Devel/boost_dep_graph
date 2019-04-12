#include "graphwidget.hpp"

#include "gui_cfg.hpp"
#include "layout.hpp"
#include "node.hpp"

#include <core/ModuleInfo.hpp>
#include <core/utils.hpp>

#include <QKeyEvent>
#include <QMarginsF>
#include <QOpenGLWidget>
#include <QSurfaceFormat>

#include <cmath>

//#define BDG_CHECK_PERF

#ifdef BDG_CHECK_PERF
#include <chrono>
#include <iostream>
#endif // BDG_CHECK_PERF

namespace mdev::bdg::gui {

namespace {
void update_position( Node*                     node,
					  const std::vector<Node*>& same_level_nodes,
					  const std::vector<Node*>& next_level_nodes,
					  const QRectF&             scene_rect )
{
	// Try to stay away from other nodes of the same level
	constexpr double rep_force     = 300; // weight of repellent forces
	constexpr auto   normalization = cfg::min_node_dist * cfg::min_node_dist * cfg::min_node_dist;

	const auto current_pos = node->get_pos();

	qreal yvel = 0;
	for( Node* other_node : same_level_nodes ) {
		if( other_node == node ) {
			continue;
		}

		const QPointF vec = other_node->get_pos() - current_pos;

		const auto dir = -sign( vec.y() );

		// we also take the distance in x direction into account as it is relevant when reordering nodes by hand
		const auto abs_dist = std::sqrt( vec.y() * vec.y() + vec.x() * vec.x() );

		yvel += dir * rep_force * normalization    //
				/ (                                //
					abs_dist * abs_dist * abs_dist //
					+ normalization );
	}

	for( Node* other_node : next_level_nodes ) {

		const QPointF vec = other_node->get_pos() - current_pos;

		const auto dir = -sign( vec.y() );

		// we also take the distance in x direction into account as it is relevant when reordering nodes by hand
		const auto abs_dist = std::sqrt( vec.y() * vec.y() + vec.x() * vec.x() );

		constexpr auto normalization2
			= cfg::min_node_dist * cfg::min_node_dist * cfg::min_node_dist * cfg::min_node_dist;

		yvel += dir * rep_force * normalization2              //
				/ (                                           //
					abs_dist * abs_dist * abs_dist * abs_dist //
					+ normalization2 );
	}

	// Node gets attracted from dependees
	constexpr double attr_force = 0.001;
	const auto       deps       = node->nodes();

	for( const Node* other_node : deps ) {
		QPointF vec = other_node->get_pos() - current_pos;
		yvel += vec.y() * attr_force;
	}

	// Calculate new position
	auto newPos = current_pos + QPointF( 0, yvel ) * 2;

	// TODO: is there a builtin for that?
	newPos.setX( std::clamp( newPos.x(), scene_rect.left(), scene_rect.right() ) );
	newPos.setY( std::clamp( newPos.y(), scene_rect.top(), scene_rect.bottom() ) );

	node->set_pos( newPos );
}

} // namespace

GraphWidget::GraphWidget( QWidget* parent )
	: QGraphicsView( parent )
	, _timer_id( 0 )
{
	auto sc = new QGraphicsScene( this );
	sc->setItemIndexMethod( QGraphicsScene::NoIndex );
	sc->setSceneRect( QRectF( QPointF {}, cfg::scene_size ) );
	setScene( sc );

	QSurfaceFormat fmt;
	fmt.setSamples( 8 );
	QSurfaceFormat::setDefaultFormat( fmt );

	setViewport( new QOpenGLWidget() );
	setBackgroundBrush( QBrush( Qt::white, Qt::SolidPattern ) );

	setCacheMode( CacheNone );
	setViewportUpdateMode( FullViewportUpdate );
	setRenderHint( QPainter::Antialiasing );
	setTransformationAnchor( AnchorUnderMouse );
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setDragMode( QGraphicsView::DragMode::ScrollHandDrag );
	fitInView( scene()->sceneRect(), Qt::KeepAspectRatio );
}

void GraphWidget::change_selected_node( Node* node )
{
	if( _selectedNode ) {
		_selectedNode->deselect();
	}
	_selectedNode = node;
	node->select();
	_edges.update_style();
}

void GraphWidget::update_all()
{
	for( auto& grp : _nodes ) {
		for( auto& node : grp ) {
			node->update();
		}
	}
	_edges.update_all();
}

void GraphWidget::keyPressEvent( QKeyEvent* event )
{
	switch( event->key() ) {
		case Qt::Key_Space: _paused = !_paused; break;
		case Qt::Key_Enter: {
			clear();
			emit( reload_requested() );
			break;
		}
		case Qt::Key_R: {
			clear();
			emit( reset_requested() );
			break;
		}
		case Qt::Key_C: {
			emit( cycle_dedection_requested() );
			break;
		}
		case Qt::Key_P: {

			QPainter pngPainter;
			QImage    image( QSize( width(), height() ), QImage::Format_ARGB32_Premultiplied ); // Format_RGB888 ); //  );
			image.fill(Qt::white);
			pngPainter.begin( &image );
			pngPainter.setRenderHint( QPainter::Antialiasing );
			scene()->render( &pngPainter );
			pngPainter.end();
			image.save( "Boostdep.png", "PNG", 50 );
			break;
		}
		case Qt::Key_Escape: {
			if( _selectedNode ) {
				_selectedNode->deselect();
				_edges.update_style();
			}
			break;
		}
		default: QGraphicsView::keyPressEvent( event );
	}
}

void GraphWidget::resizeEvent( QResizeEvent* event )
{
	fitInView( scene()->sceneRect(), Qt::KeepAspectRatio );
	event->accept();
}

void GraphWidget::update_positions()
{
	if( !_paused ) {
		auto area = scene()->sceneRect().marginsRemoved( cfg::margins );
		for( int level = 0; level < _nodes.size(); ++level ) {
			auto& group = _nodes[level];
			for( auto* node : group ) {
				if( scene()->mouseGrabberItem() == node ) {
					continue;
				}
				if( level > 0 ) {
					update_position( node, group, _nodes[level - 1], area );
				} else {
					update_position( node, group, std::vector<Node*> {}, area );
				}
			}
		}
	}
	_edges.update_positions();
} // namespace mdev::bdg::gui

void GraphWidget::timerEvent( QTimerEvent* )
{
	update_positions();

#ifdef BDG_CHECK_PERF

	using namespace std::chrono;
	using namespace std::chrono_literals;
	static auto last = std::chrono::system_clock::now();
	static auto cnt  = 0;
	cnt++;
	if( ( system_clock::now() - last ) > 5s ) {
		std::cout << cnt / ( ( system_clock::now() - last ) / 1s ) << "\n";
		last = system_clock::now();
		cnt  = 0;
	}
#endif // BDG_CHECK_PERF
}

void GraphWidget::wheelEvent( QWheelEvent* event )
{
	auto factor = pow( 2.0, event->delta() / 240.0 );
	scale( factor, factor );
	event->accept();
}

void GraphWidget::set_data( modules_data* modules )
{
	clear();

	auto max_level = std::max_element( modules->begin(), modules->end(), []( auto& l, auto& r ) {
						 return l.second.level < r.second.level;
					 } )->second.level;

	auto layout = gui::layout_boost_modules( *modules, this->sceneRect().marginsRemoved( cfg::margins ) );

	std::map<std::string, Node*> module_node_map;

	for( auto& [name, info] : *modules ) {
		int  z_level          = max_level + 2 - info.level;
		auto n                = new Node( this, &info, z_level );
		module_node_map[name] = n;

		if( info.level >= _nodes.size() ) {
			_nodes.resize( info.level + 1 );
		}
		_nodes[info.level].push_back( n );
		auto pos = layout[name];
		scene()->addItem( n );
		n->set_pos( pos );
	}

	std::vector<std::pair<Node*, Node*>> connections;

	for( auto& [module, node] : module_node_map ) {
		Node* src = module_node_map.at( module );
		for( auto* d : node->info()->deps ) {
			Node* dest = module_node_map.at( d->name );
			if( dest->info()->level != src->info()->level ) {
				dest->add_attracting_node( src );
			}
			connections.emplace_back( src, dest );
		}
	}

	// get the layout into a (mostly) stable state first
	for( int i = 0; i < 5000; ++i ) {
		update_positions();
	}

	_edges.create_edges( *scene(), connections );

	_timer_id = startTimer( 1000 / 30 );
}

void GraphWidget::clear()
{
	killTimer( _timer_id );
	_timer_id     = 0;
	_selectedNode = nullptr;
	_nodes.clear();
	_edges.clear();
	scene()->clear();
}

} // namespace mdev::bdg::gui
