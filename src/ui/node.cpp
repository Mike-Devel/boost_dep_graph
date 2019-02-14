#include "node.hpp"
#include "graphwidget.hpp"
#include "gui_cfg.hpp"

#include <core/utils.hpp>
#include <core/analysis.hpp>


#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>

namespace mdev::bdg::gui {

Node::Node( GraphWidget* graphWidget, ModuleInfo* module, int z_level )
	: _graph( graphWidget )
	, _moduleInfo( module )
	, _name( QString::fromStdString( module->name ) )
{
	setFlag( ItemIsMovable );
	setFlag( ItemSendsGeometryChanges );
	setCacheMode( DeviceCoordinateCache );
	setZValue( z_level );

	QFontMetrics fm( cfg::module_name_font );
	_text_size             = fm.size(0, _name );
}

void Node::addNode( const Node* node )
{
	_node_list.push_back( node );
}

mdev::span<const Node* const> Node::nodes() const
{
	return {_node_list};
}


QRectF Node::boundingRect() const
{
	QRectF node_rect = QRectF( -cfg::node_radius, -cfg::node_radius, cfg::node_radius*2, cfg::node_radius*2 );
	QRectF text_rect
		= QRectF( QPointF( -cfg::node_radius, -cfg::node_radius - _text_size.height() * 2 ), _text_size * 2 );

	return node_rect.united( text_rect );
}

QPainterPath Node::shape() const
{
	QPainterPath path;
	path.addEllipse( -cfg::node_radius, -cfg::node_radius, cfg::node_radius * 2, cfg::node_radius*2 );
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
	painter->drawEllipse( -cfg::node_radius, -cfg::node_radius, cfg::node_radius * 2, cfg::node_radius*2 );

	//double scaleValue = scale() / painter->transform().m11();
	//painter->save();
	//painter->scale( scaleValue, scaleValue );
	painter->setFont( cfg::module_name_font );
	painter->setPen( QPen( Qt::black, 3 ) );
	painter->drawText( QPointF{-cfg::node_radius, -cfg::node_radius}, _name );

	//painter->restore();
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

} // namespace mdev::bdg::gui
