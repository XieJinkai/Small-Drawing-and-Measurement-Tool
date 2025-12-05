#include "CanvasView.h"
#include <QScrollBar>

CanvasView::CanvasView(QWidget* parent) : QGraphicsView(parent) {
    init();
}

CanvasView::CanvasView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent) {
    init();
}

void CanvasView::init() {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag); // Custom handling
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    _isPanning = false;
}

void CanvasView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double scaleFactor = 1.15;
        if (event->angleDelta().y() < 0) {
            scaleFactor = 1.0 / scaleFactor;
        }
        setZoom(scaleFactor);
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void CanvasView::setZoom(double scaleFactor) {
    scale(scaleFactor, scaleFactor);
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        _isPanning = true;
        _lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
    if (_isPanning) {
        QPoint delta = event->pos() - _lastMousePos;
        _lastMousePos = event->pos();
        
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        _isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}
