#pragma once

#include <QGraphicsView>

#include "graph_scene.h"
#include "node.h"

namespace node_editor {

/// a simple graph widget
class GraphWidget : public QGraphicsView {
	Q_OBJECT

  public:
	GraphWidget(QWidget* parent = NULL);

	GraphScene& scene();

  protected:
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;

  private:
	GraphScene m_scene;

	int m_mouseX, m_mouseY;
};

}  // namespace node_editor
