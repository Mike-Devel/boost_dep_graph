#include "node.hpp"
#include "edge.hpp"
#include "graphwidget.hpp"

#include "../ModuleInfo.hpp"
#include "../analysis.hpp"

#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>


namespace mdev::bdg {

Node::Node( GraphWidget* graphWidget, ModuleInfo* module, int z_level )
	: _graph( graphWidget )
	, _moduleInfo( module )
	, _name( QString::fromStdString( module->name ) )
{
	setFlag( ItemIsMovable );
	setFlag( ItemSendsGeometryChanges );
	setCacheMode( DeviceCoordinateCache );
	setZValue( z_level );
}

void Node::addEdge( Edge* edge )
{
	_edgeList.push_back( edge );
	edge->adjust();
}

std::vector<Edge*> Node::edges() const
{
	return _edgeList;
}

namespace {

template<typename T>
int sign( T val )
{
	return ( T( 0 ) < val ) - ( val < T( 0 ) );
}

} // namespace

QRectF Node::boundingRect() const
{
	qreal adjust = 2;
	return QRectF( -10 - adjust, -30 - adjust, 150 + adjust, 50 + adjust );
}

QPainterPath Node::shape() const
{
	QPainterPath path;
	path.addEllipse( -10, -10, 20, 20 );
	return path;
}

void Node::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* )
{
	painter->setPen( Qt::NoPen );

	QRadialGradient gradient( -3, -3, 10 );
	if( option->state & QStyle::State_Sunken ) {
		gradient.setCenter( 3, 3 );
		gradient.setFocalPoint( 3, 3 );
		gradient.setColorAt( 0, QColor( Qt::darkYellow ) );
		gradient.setColorAt( 1, QColor( Qt::yellow ) );
	} else {
		if( this->_moduleInfo->has_cmake ) {
			gradient.setColorAt( 0, Qt::green );
			gradient.setColorAt( 1, Qt::darkGreen );
		} else {
			if( this->_moduleInfo->deps_have_cmake ) {
				gradient.setColorAt( 0, Qt::yellow );
				gradient.setColorAt( 1, Qt::darkYellow );
			} else {
				gradient.setColorAt( 0, Qt::red );
				gradient.setColorAt( 1, Qt::darkRed );
			}
		}
	}
	painter->setBrush( gradient );

	painter->setPen( QPen( Qt::black, 0 ) );
	painter->drawEllipse( -10, -10, 20, 20 );
	painter->setFont( QFont( "Helvetica", 15 ) );
	painter->setPen( QPen( Qt::black, 3 ) );
	painter->drawText( QPoint{-10, -10}, _name );
}

void Node::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
	// right button toggles cmake info
	if( event->button() == Qt::RightButton ) {
		_moduleInfo->has_cmake = !_moduleInfo->has_cmake;

		for( auto info : _moduleInfo->all_rev_deps ) {
			update_cmake_status( *info );
		}
	} else {
		QGraphicsItem::mousePressEvent( event );
		_graph->change_selected_node( this );
	}
	update();
}

void Node::mouseReleaseEvent( QGraphicsSceneMouseEvent* event )
{
	QGraphicsItem::mouseReleaseEvent( event );
	update();
}

} // namespace mdev::bdg
