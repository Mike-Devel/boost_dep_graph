/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
**               2018-2019 Mike Dev
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/

#include "edge.hpp"
#include "node.hpp"

#include "../ModuleInfo.hpp"

#include <QPainter>
#include <qmath.h>

namespace mdev::bdg {

Edge::Edge( Node* sourceNode, Node* destNode )
	: arrowSize( 10 )
	, source( sourceNode )
	, dest( destNode )
{
	setAcceptedMouseButtons( 0 );
	source->addEdge( this );
	dest->addEdge( this );
	adjust();
}

Node* Edge::sourceNode() const
{
	return source;
}

Node* Edge::destNode() const
{
	return dest;
}

void Edge::adjust()
{
	if( !source || !dest ) return;

	QLineF line( mapFromItem( source, 0, 0 ), mapFromItem( dest, 0, 0 ) );
	qreal  length = line.length();

	prepareGeometryChange();

	if( length > qreal( 20. ) ) {
		QPointF edgeOffset( ( line.dx() * 10 ) / length, ( line.dy() * 10 ) / length );
		sourcePoint = line.p1() + edgeOffset;
		destPoint   = line.p2() - edgeOffset;
	} else {
		sourcePoint = destPoint = line.p1();
	}
}

QRectF Edge::boundingRect() const
{
	if( !source || !dest ) return QRectF();

	qreal penWidth = 1;
	qreal extra    = ( penWidth + arrowSize ) / 2.0;

	return QRectF( sourcePoint, QSizeF( destPoint.x() - sourcePoint.x(), destPoint.y() - sourcePoint.y() ) )
		.normalized()
		.adjusted( -extra, -extra, extra, extra );
}

void draw_arrow_head( QPainter* painter, QPointF head, double angle )
{
	double size             = 10;
	double half_width_angle = M_PI / 3 / 2;

	QPointF t1 = head - QPointF( cos( angle - half_width_angle ) * size, sin( angle - half_width_angle ) * size );
	QPointF t2 = head - QPointF( cos( angle + half_width_angle ) * size, sin( angle + half_width_angle ) * size );

	painter->drawPolygon( QPolygonF( {head, t1, t2} ) );
}

void Edge::paint( QPainter* painter, const QStyleOptionGraphicsItem*, QWidget* )
{
	if( !source || !dest ) return;

	QLineF line( sourcePoint, destPoint );
	if( qFuzzyCompare( line.length(), qreal( 0. ) ) ) return;

	// defaults
	QColor line_color = Qt::darkGray;
	int    width      = 1;
	this->setZValue( -2 );

	// special highliting formatting
	if( source->is_selected() || dest->is_selected() ) {
		width = 4;
		this->setZValue( 1 );
	}
	if( !dest->info()->has_cmake ) {
		line_color = Qt::red;
	}

	// Draw the line itself
	painter->setPen( QPen( line_color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
	painter->drawLine( line );

	// Draw the arrows
	double angle = std::atan2( line.dy(), line.dx() );

	painter->setBrush( line_color );
	draw_arrow_head( painter, destPoint, angle );
}

} // namespace mdev::bdg