#pragma once

#include <QSize>
#include <QGraphicsItem>
#include <QString>
#include <vector>

#include <core/utils.hpp>

class QGraphicsSceneMouseEvent;

namespace mdev::bdg {

struct ModuleInfo;

namespace gui {

class GraphWidget;

class Node : public QGraphicsItem {
public:
	Node( GraphWidget* graphWidget, ModuleInfo* module, int z_level );

	QRectF       boundingRect() const override;
	QPainterPath shape() const override;
	void         paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget ) override;

	void                          add_attracting_node( const Node* );
	mdev::span<const Node* const> nodes() const;

	void select() noexcept { _is_selected = true; }
	void deselect() noexcept { _is_selected = false; }
	bool is_selected() const noexcept { return _is_selected; }

	ModuleInfo* info() const noexcept { return _moduleInfo; }

	// to prevent continuous redrawing, we don't forware every update to the underlying QGraphicsItem
	void    set_pos( QPointF new_pos );
	QPointF get_pos() const { return _pos; }

protected:
	void mousePressEvent( QGraphicsSceneMouseEvent* event ) override;
	void mouseReleaseEvent( QGraphicsSceneMouseEvent* event ) override;
	QVariant itemChange( QGraphicsItem::GraphicsItemChange change, const QVariant& value ) override;

private:
	QPointF                  _pos{0,0};
	QString                  _name;
	QSize                    _text_size;
	std::vector<const Node*> _node_list; // other nodes that attract this node

	GraphWidget* _graph       = nullptr;
	ModuleInfo*  _moduleInfo  = nullptr;
	bool         _is_selected = false;
};

} // namespace gui

} // namespace mdev::bdg
