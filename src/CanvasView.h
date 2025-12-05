#ifndef CANVASVIEW_H
#define CANVASVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>

class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    CanvasView(QWidget* parent = nullptr);
    CanvasView(QGraphicsScene* scene, QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void init();
    void setZoom(double scaleFactor);

    bool _isPanning;
    QPoint _lastMousePos;
};

#endif // CANVASVIEW_H
