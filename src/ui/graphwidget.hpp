#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>
#include <vector>
#include "edges.hpp"

namespace mdev::bdg {

struct ModuleInfo;

namespace gui {

class Node;

class GraphWidget : public QGraphicsView {
	Q_OBJECT

public:
	GraphWidget( QWidget* parent = nullptr );

	void set_data( const std::vector<std::vector<ModuleInfo*>>& layout );

public slots:
	void change_selected_node( Node* );
	void clear();

signals:
	void reload_requested();

protected:
	void keyPressEvent( QKeyEvent* event ) override;
	void timerEvent( QTimerEvent* event ) override;
	void wheelEvent( QWheelEvent* event ) override;

private:
	void update_positions();

	std::vector<std::vector<Node*>> _nodes;
	QGraphicsScene*                 _scene = nullptr;
	Edges                           _edges;

	int   _timer_id{};
	Node* _selectedNode = nullptr;
};
} // namespace gui
} // namespace mdev::bdg
