#include "graphwidget.hpp"

#include "edge.hpp"
#include "node.hpp"

#include "../ModuleInfo.hpp"
#include "../utils.hpp"

#include <math.h>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTreeWidget>
#include <iostream>
#include <random>

namespace mdev {
namespace bdg {

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
}

void GraphWidget::change_selected_node( Node* node )
{
	if( _selectedNode ) {
		_selectedNode->deselect();
	}
	_selectedNode = node;
	node->select();
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

void GraphWidget::timerEvent( QTimerEvent* )
{
	for( auto&& group : _nodes ) {
		for( auto node : group ) {
			updatePosition( node, group );
		}
	}
}

void GraphWidget::wheelEvent( QWheelEvent* event )
{
	auto factor = pow( 2.0, -event->delta() / 240.0 );
	scale( factor, factor );
}

void GraphWidget::set_data( const std::vector<std::vector<ModuleInfo*>>& layout )
{
	clear();

	std::map<std::string, Node*> module_node;

	int       xpos = _scene->sceneRect().x();
	const int dx   = _scene->sceneRect().width() / layout.size();
	for( auto& group : layout ) {
		_nodes.push_back( {} );
		int ypos = 0;
		int dy   = 1;

		if( group.size() > 1 ) {
			ypos = _scene->sceneRect().y();
			dy   = _scene->sceneRect().height() / ( group.size() - 1 );
		} else {
			ypos = _scene->sceneRect().height() / 2;
		}

		for( auto& module : group ) {
			auto n = new Node( this, module, layout.size() + 2 - module->level );

			module_node[module->name] = n;
			_nodes.back().push_back( n );

			n->setPos( xpos, ypos );
			_scene->addItem( n );
			ypos += dy;
		}

		xpos += dx;
	}


	for( auto& [module, node] : module_node ) {

		for( auto* d : node->info()->deps ) {
			auto e = new Edge( module_node.at( module ), module_node.at( d->name ) );
			_edges.push_back( e );
			_scene->addItem( e );
		}
	}
	_timer_id = startTimer( 1000 / 40 );
}

void GraphWidget::clear()
{
	killTimer( _timer_id );
	_timer_id = 0;
	_nodes.clear();
	_edges.clear();
	_selectedNode = nullptr;
	_scene->clear();
}

namespace {

template<typename T>
int sign( T val )
{
	return ( T( 0 ) < val ) - ( val < T( 0 ) );
}

} // namespace

void GraphWidget::updatePosition( Node* node, const std::vector<Node*>& other )
{
	if( !_scene || _scene->mouseGrabberItem() == node ) {
		return ;
	}

	// Try to stay away from other nodes of the same level
	qreal yvel = 0;
	for( Node* other_node : other ) {
		if( other_node == node ) {
			continue;
		}

		QPointF vec = other_node->pos() - node->pos();

		auto dir = -sign( vec.y() );

		auto dy_abs = std::sqrt( vec.y() * vec.y() );

		const double min_dist = 30;
		const double force    = 100;

		yvel += dir * force                      //
				* min_dist * min_dist * min_dist //
				/ ( dy_abs * dy_abs * dy_abs + min_dist * min_dist * min_dist );
	}

	// Node gets attracted from dependees
	double weight = 0.01 / ( node->edges().size() + 1 );
	for( Edge* edge : node->edges() ) {
		if( edge->destNode() == node ) {
			QPointF vec = edge->sourceNode()->pos() - node->pos();
			yvel += vec.y() * weight;
		}
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

	for( auto* edge : _edges ) {
		edge->adjust();
	}
}

} // namespace bdg
} // namespace mdev
