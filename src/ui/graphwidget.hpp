#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>
#include <vector>

namespace mdev::bdg {

struct ModuleInfo;

class Node;
class Edge;

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
	void updatePosition( Node* node, const std::vector<Node*>& other );

	std::vector<std::vector<Node*>> _nodes;
	std::vector<Edge*>              _edges;
	QGraphicsScene*                 _scene = nullptr;

	int   _timer_id{};
	Node* _selectedNode = nullptr;
};

} // namespace mdev::bdg
