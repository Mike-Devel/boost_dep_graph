#include "graphwidget.hpp"


#include "node.hpp"

#include <core/ModuleInfo.hpp>
#include <core/utils.hpp>

#include <QKeyEvent>
#include <QMarginsF>
#include <QSurfaceFormat>
#include <QOpenGLWidget>

#include <cmath>

#define BDG_CHECK_PERF

#ifdef BDG_CHECK_PERF
#include <chrono>
#include <iostream>
#endif // BDG_CHECK_PERF


namespace mdev::bdg::gui {

namespace {
void update_position( Node* node, const std::vector<Node*>& other, const QRectF& scene_rect )
{
	// Try to stay away from other nodes of the same level
	constexpr double min_dist  = 20;  // ~The minimum distance between two nodes we try to hold
	constexpr double rep_force = 100; // weight of repellent forces

	const auto current_pos = node->pos();

	qreal yvel = 0;
	for( Node* other_node : other ) {
		if( other_node == node ) {
			continue;
		}

		const QPointF vec = other_node->pos() - current_pos;

		const auto dir = -sign( vec.y() );

		// we also take the distance in x direction into account as it is relevant when reordering nodes by hand
		const auto abs_dist = std::sqrt( vec.y() * vec.y() + vec.x() * vec.x() );

		yvel += dir * rep_force                    //
				* min_dist * min_dist * min_dist   //
				/ (                                //
					abs_dist * abs_dist * abs_dist //
					+ min_dist * min_dist * min_dist );
	}

	// Node gets attracted from dependees
	constexpr double attr_force = 0.001;
	const auto       deps       = node->nodes();

	for( const Node* other_node : deps ) {
		QPointF vec = other_node->pos() - current_pos;
		yvel += vec.y() * attr_force;
	}

	// Calculate new position
	auto newPos = current_pos + QPointF( 0, yvel ) * 2;

	// TODO: is there a builtin for that?
	newPos.setX( std::clamp( newPos.x(), scene_rect.left(), scene_rect.right() ) );
	newPos.setY( std::clamp( newPos.y(), scene_rect.top(), scene_rect.bottom() ) );

	node->setPos( newPos );
}

} // namespace

GraphWidget::GraphWidget( QWidget* parent )
	: QGraphicsView( parent )
	, _timer_id( 0 )
{
	_scene = new QGraphicsScene( this );
	_scene->setItemIndexMethod( QGraphicsScene::NoIndex );
	_scene->setSceneRect( -900, -550, 1800, 1100 );
	setScene( _scene );

	QSurfaceFormat fmt;
	fmt.setSamples( 4 );
	QSurfaceFormat::setDefaultFormat( fmt );

	setViewport( new QOpenGLWidget());
	setBackgroundBrush( QBrush( Qt::white, Qt::SolidPattern ) );

	setCacheMode( CacheNone );
	setViewportUpdateMode( FullViewportUpdate );
	setRenderHint( QPainter::Antialiasing );
	setTransformationAnchor( AnchorUnderMouse );
	setDragMode( QGraphicsView::DragMode::ScrollHandDrag );

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

void GraphWidget::keyPressEvent( QKeyEvent* event )
{
	switch( event->key() ) {
		case Qt::Key_Space:
			clear();
			emit( reload_requested() );
			break;
		default: QGraphicsView::keyPressEvent( event );
	}
}

void GraphWidget::update_positions()
{
	constexpr QMarginsF margins( 10, 10, 10, 10 );

	auto area = _scene->sceneRect().marginsRemoved( margins );

	for( auto&& group : _nodes ) {
		for( auto* node : group ) {
			if( _scene->mouseGrabberItem() == node ) {
				continue;
			}
			update_position( node, group, area );
		}
	}
	_edges.update_positions();
}

void GraphWidget::timerEvent( QTimerEvent* )
{
	update_positions();

#ifdef BDG_CHECK_PERF


	using namespace std::chrono;
	using namespace std::chrono_literals;
	static auto last = std::chrono::system_clock::now();
	static auto cnt  = 0;
	if( ++cnt >= 100 ) {
		cnt = 0;
		std::cout << 100 * 1s / ( system_clock::now() - last ) << "\n";
		last = system_clock::now();
	}
#endif // BDG_CHECK_PERF
}

void GraphWidget::wheelEvent( QWheelEvent* event )
{
	auto factor = pow( 2.0, event->delta() / 240.0 );
	scale( factor, factor );
	event->accept();
}

void GraphWidget::set_data( const std::vector<std::vector<ModuleInfo*>>& layout )
{
	clear();

	std::map<std::string, Node*> module_node_map;

	int       xpos = _scene->sceneRect().x();
	const int dx   = _scene->sceneRect().width() / layout.size();
	for( auto& group : layout ) {

		// determine parameters for iteration (inital position  and position
		int ypos = 0;
		int dy   = 1;

		if( group.size() > 1 ) {
			ypos = _scene->sceneRect().y();
			dy   = _scene->sceneRect().height() / ( group.size() - 1 );
		} else {
			ypos = _scene->sceneRect().height() / 2;
		}

		_nodes.push_back( {} );
		for( auto& module : group ) {
			int  z_level = static_cast<int>( layout.size() ) + 2 - module->level;
			auto n       = new Node( this, module, z_level );

			module_node_map[module->name] = n;
			_nodes.back().push_back( n );

			n->setPos( xpos, ypos );
			_scene->addItem( n );
			ypos += dy;
		}

		xpos += dx;
	}

	std::vector<std::pair<Node*, Node*>> connections;

	for( auto& [module, node] : module_node_map ) {
		Node* src = module_node_map.at( module );
		for( auto* d : node->info()->deps ) {
			Node* dest = module_node_map.at( d->name );
			dest->addNode( src );
			connections.emplace_back( src, dest );
		}
	}

	// get the layout into a (mostly) stable state first
	for( int i = 0; i < 200; ++i ) {
		update_positions();
	}

	_edges.create_edges( *_scene, connections );

	_timer_id = startTimer( 1000 / 20 );
}

void GraphWidget::clear()
{
	killTimer( _timer_id );
	_timer_id     = 0;
	_selectedNode = nullptr;
	_nodes.clear();
	_edges.clear();
	_scene->clear();
}

} // namespace mdev::bdg::gui
