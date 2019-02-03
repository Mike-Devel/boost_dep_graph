#pragma once

#include <QGraphicsItem>
#include <QString>
#include <vector>

class QGraphicsSceneMouseEvent;

namespace mdev::bdg {

struct ModuleInfo;

class Edge;
class GraphWidget;

class Node : public QGraphicsItem {
public:
	Node( GraphWidget* graphWidget, ModuleInfo* module, int z_level );

	QRectF       boundingRect() const override;
	QPainterPath shape() const override;
	void         paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget ) override;

	void               addEdge( Edge* edge );
	std::vector<Edge*> edges() const;

	void select() noexcept { _is_selected = true; }
	void deselect() noexcept { _is_selected = false; }
	bool is_selected() const noexcept { return _is_selected; }

	ModuleInfo* info() const noexcept { return _moduleInfo; }

protected:
	void mousePressEvent( QGraphicsSceneMouseEvent* event ) override;
	void mouseReleaseEvent( QGraphicsSceneMouseEvent* event ) override;

private:
	QString            _name;
	std::vector<Edge*> _edgeList;
	GraphWidget*       _graph       = nullptr;
	ModuleInfo*        _moduleInfo  = nullptr;
	bool               _is_selected = false;
};

} // namespace mdev::bdg
