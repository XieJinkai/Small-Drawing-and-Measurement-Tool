#ifndef CANVASSCENE_H
#define CANVASSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QList>
#include <QPointF>

static constexpr double PI = 3.1415926535897932384626433832795;

enum class ToolMode {
    None,
    Line,
    Arc,
    Angle,
    Distance
};

class CanvasScene : public QGraphicsScene {
    Q_OBJECT

public:
    CanvasScene(QObject* parent = nullptr);
    void setMode(ToolMode mode);
    void setScaleRatio(double pxPerMm); // pixels per 1mm
    void loadImage(const QString& path);
    void clearMeasurements();
    double getScaleRatio() const { return scaleRatio; }

signals:
    void messageChanged(const QString& msg);
    void modeChanged(ToolMode mode);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void finishLine(const QPointF& p1, const QPointF& p2);
    void finishArc(const QPointF& p1, const QPointF& p2, const QPointF& p3);
    void finishAngle(const QPointF& p1, const QPointF& p2, const QPointF& p3);
    void finishDistance(QGraphicsLineItem* line, const QPointF& point);
    
    void updateMeasurements();
    void updatePreview(const QPointF& pos);

    // Helper to calculate distance in mm
    double toMm(double pixels) const;
    

    ToolMode currentMode;
    double scaleRatio; // pixels / mm
    
    QList<QPointF> currentPoints;
    QGraphicsLineItem* tempLine;
    QGraphicsPathItem* tempArc; // Preview for arc
    QGraphicsLineItem* selectedLineForDist; // The line selected for distance measurement
    
    // Store items to update them later
    struct MeasureItem {
        int type; // 0: Line, 1: Arc, 2: Distance
        QGraphicsItem* graphicsItem; // The main visual item (Line, Path, etc.)
        QGraphicsSimpleTextItem* textItem;
        // Store geometry data to recalculate
        QList<QPointF> points; 
        QList<QGraphicsItem*> extraItems;
    };
    QList<MeasureItem> measureItems;
};

#endif // CANVASSCENE_H
