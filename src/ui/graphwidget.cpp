#include "graphwidget.hpp"

#include "node.hpp"

#include "../ModuleInfo.hpp"
#include "../utils.hpp"

#include <QKeyEvent>

#include <cmath>
#include <chrono>
#include <iostream>

namespace mdev::bdg::gui {

GraphWidget::GraphWidget( QWidget* parent )
	: QGraphicsView( parent )
	, _timer_id( 0 )
{
	_scene = new QGraphicsScene( this );
	_scene->setItemIndexMethod( QGraphicsScene::NoIndex );
	_scene->setSceneRect( -800, -500, 1800, 1100 );
	setScene( _scene );

	setCacheMode( CacheNone );
	setViewportUpdateMode( FullViewportUpdate );
	setRenderHint( QPainter::Antialiasing );
	setTransformationAnchor( AnchorUnderMouse );
	setMinimumSize( 1800, 1000 );
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

void GraphWidget::update_positions() {
	for( auto&& group : _nodes ) {
		for( auto* node : group ) {
			updatePosition( node, group );
		}
	}
	_edges.update_positions();

}

void GraphWidget::timerEvent( QTimerEvent* )
{
	update_positions();

	using namespace std::chrono;
	using namespace std::chrono_literals;
	static auto last = std::chrono::system_clock::now();
	static auto cnt  = 0;
	if( ++cnt >= 100 ) {
		cnt = 0;
		std::cout << 100 * 1s/ (system_clock::now() - last) << "\n";
		last = system_clock::now();
	}
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
			auto n = new Node( this, module, z_level );

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
	_timer_id = 0;
	_selectedNode = nullptr;
	_nodes.clear();
	_edges.clear();
	_scene->clear();
}

void GraphWidget::updatePosition( Node* node, const std::vector<Node*>& other )
{
	if( !_scene || _scene->mouseGrabberItem() == node ) {
		return;
	}

	// Try to stay away from other nodes of the same level
	qreal yvel = 0;
	for( Node* other_node : other ) {
		if( other_node == node ) {
			continue;
		}

		QPointF vec = other_node->pos() - node->pos();

		auto dir = -sign( vec.y() );

		// we also take the distance in x direction into account as it is relevant when reordering nodes by hand
		auto abs_dist = std::sqrt( vec.y() * vec.y() + vec.x() * vec.x() );

		const double min_dist = 20;
		const double force    = 100;

		yvel += dir * force                        //
				* min_dist * min_dist * min_dist   //
				/ (                                //
					abs_dist * abs_dist * abs_dist //
					+ min_dist * min_dist * min_dist );
	}

	// Node gets attracted from dependees
	auto   deps = node->nodes();
	double weight    = 0.01 / ( deps.size() + 1 );
	for( const Node* n : deps ) {
		QPointF vec = n->pos() - node->pos();
		yvel += vec.y() * weight;
	}

	auto newPos = node->pos() + QPointF( 0, yvel ) * 2;

	// Lets only place leaf nodes on the border
	const auto LowerBound = scene()->sceneRect().top() + 10;
	const auto UpperBound = scene()->sceneRect().bottom() - 10;
	const auto LeftBound  = scene()->sceneRect().left() + 10;
	const auto RightBound = scene()->sceneRect().right() - 10;

	newPos.setX( std::clamp( newPos.x(), LeftBound, RightBound ) );
	newPos.setY( std::clamp( newPos.y(), LowerBound, UpperBound ) );

	node->setPos( newPos );
}

} // namespace mdev::bdg::gui
