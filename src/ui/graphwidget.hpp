#pragma once

#include "edges.hpp"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <vector>

namespace mdev::bdg {

struct ModuleInfo;
struct modules_data;

namespace gui {

class Node;

class GraphWidget : public QGraphicsView {
	Q_OBJECT

public:
	GraphWidget( QWidget* parent = nullptr );

	void set_data( mdev::bdg::modules_data* modules );

public slots:
	void change_selected_node( Node* );
	void clear();
	void update_all();

signals:
	void reload_requested();
	void reprint_stats_requested();
	void reset_requested();
	void cycle_dedection_requested();

protected:
	void keyPressEvent( QKeyEvent* event ) override;
	void timerEvent( QTimerEvent* event ) override;
	void wheelEvent( QWheelEvent* event ) override;
	void resizeEvent( QResizeEvent* event ) override;

private:
	void update_positions();

	std::vector<std::vector<Node*>> _nodes;
	Edges                           _edges;

	int   _timer_id{};
	Node* _selectedNode = nullptr;
	bool  _paused       = false;
};
} // namespace gui
} // namespace mdev::bdg
