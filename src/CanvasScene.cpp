#include "CanvasScene.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QPainter>
#include <cmath>
#include <QDebug>

// Helper for Euclidean distance
double dist(QPointF p1, QPointF p2) {
    return std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2));
}

CanvasScene::CanvasScene(QObject* parent)
    : QGraphicsScene(parent), currentMode(ToolMode::None), scaleRatio(1.0), tempLine(nullptr), tempArc(nullptr), selectedLineForDist(nullptr) {
}

void CanvasScene::setMode(ToolMode mode) {
    currentMode = mode;
    currentPoints.clear();
    if (tempLine) {
        removeItem(tempLine);
        delete tempLine;
        tempLine = nullptr;
    }
    if (tempArc) {
        removeItem(tempArc);
        delete tempArc;
        tempArc = nullptr;
    }
    selectedLineForDist = nullptr;
    // Reset selection
    foreach(QGraphicsItem* item, items()) {
        item->setSelected(false);
    }
}

void CanvasScene::setScaleRatio(double pxPerMm) {
    if (pxPerMm > 0) {
        scaleRatio = pxPerMm;
        updateMeasurements();
    }
}

void CanvasScene::updateMeasurements() {
    for (const auto& item : measureItems) {
        if (item.type == 0) { // Line
            double lengthPx = dist(item.points[0], item.points[1]);
            double lengthMm = toMm(lengthPx);
            item.textItem->setText(QString::number(lengthMm, 'f', 2) + " mm");
        } else if (item.type == 1) { // Arc (Radius)
             // Re-calculate radius from 3 points
             QPointF p1 = item.points[0];
             QPointF p2 = item.points[1];
             QPointF p3 = item.points[2];
             
             double x1 = p1.x(), y1 = p1.y();
             double x2 = p2.x(), y2 = p2.y();
             double x3 = p3.x(), y3 = p3.y();
             double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
             if (std::abs(D) > 1e-5) {
                 double center_x = ((x1*x1 + y1*y1) * (y2 - y3) + (x2*x2 + y2*y2) * (y3 - y1) + (x3*x3 + y3*y3) * (y1 - y2)) / D;
                 double center_y = ((x1*x1 + y1*y1) * (x3 - x2) + (x2*x2 + y2*y2) * (x1 - x3) + (x3*x3 + y3*y3) * (x2 - x1)) / D;
                 QPointF center(center_x, center_y);
                 double radiusPx = dist(center, p1);
                 double radiusMm = toMm(radiusPx);
                 item.textItem->setText(QString("半径: %1 mm").arg(radiusMm, 0, 'f', 2));
             }
        } else if (item.type == 2) { // Distance
             // Point 0 is target point, Point 1 is projection point
             double distPx = dist(item.points[0], item.points[1]);
             double distMm = toMm(distPx);
             item.textItem->setText(QString("距离: %1 mm").arg(distMm, 0, 'f', 2));
        }
    }
}

void CanvasScene::loadImage(const QString& path) {
    clear();
    measureItems.clear();
    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        addPixmap(pixmap);
        setSceneRect(pixmap.rect());
    }
    setMode(ToolMode::None);
    emit modeChanged(ToolMode::None);
}

double CanvasScene::toMm(double pixels) const {
    return pixels / scaleRatio;
}


void CanvasScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QPointF pos = event->scenePos();

    if (currentMode == ToolMode::Line) {
        currentPoints.append(pos);
        if (currentPoints.size() == 1) {
            emit messageChanged("点击第二个点以完成直线");
        } else if (currentPoints.size() == 2) {
            finishLine(currentPoints[0], currentPoints[1]);
            currentPoints.clear();
            if (tempLine) { removeItem(tempLine); delete tempLine; tempLine = nullptr; }
            emit messageChanged("直线绘制完成。");
            setMode(ToolMode::None);
            emit modeChanged(ToolMode::None);
        }
    } 
    else if (currentMode == ToolMode::Arc) {
        currentPoints.append(pos);
        if (currentPoints.size() == 1) {
            emit messageChanged("点击圆弧第二个点");
        } else if (currentPoints.size() == 2) {
            emit messageChanged("点击圆弧第三个点");
        } else if (currentPoints.size() == 3) {
            finishArc(currentPoints[0], currentPoints[1], currentPoints[2]);
            currentPoints.clear();
            if (tempArc) { removeItem(tempArc); delete tempArc; tempArc = nullptr; }
            emit messageChanged("圆弧绘制完成。");
            setMode(ToolMode::None);
            emit modeChanged(ToolMode::None);
        }
    }
    else if (currentMode == ToolMode::Angle) {
        currentPoints.append(pos);
        if (currentPoints.size() == 1) {
            emit messageChanged("点击顶点");
        } else if (currentPoints.size() == 2) {
            emit messageChanged("点击终点");
        } else if (currentPoints.size() == 3) {
            finishAngle(currentPoints[0], currentPoints[1], currentPoints[2]);
            currentPoints.clear();
            if (tempLine) { removeItem(tempLine); delete tempLine; tempLine = nullptr; }
            emit messageChanged("角度测量完成。");
            setMode(ToolMode::None);
            emit modeChanged(ToolMode::None);
        }
    }
    else if (currentMode == ToolMode::Distance) {
        if (!selectedLineForDist) {
            // Try to select a line with 5px tolerance
            QRectF selectionRect(pos.x() - 5, pos.y() - 5, 10, 10);
            QList<QGraphicsItem*> itemsInRect = items(selectionRect);
            
            QGraphicsLineItem* lineItem = nullptr;
            for (QGraphicsItem* item : itemsInRect) {
                lineItem = dynamic_cast<QGraphicsLineItem*>(item);
                if (lineItem) break;
            }

            if (lineItem) {
                selectedLineForDist = lineItem;
                // Highlight it?
                QPen pen = lineItem->pen();
                pen.setColor(Qt::magenta);
                pen.setWidth(pen.width() + 2);
                lineItem->setPen(pen);
                emit messageChanged("直线已选中。点击一个点以测量距离。");
            } else {
                emit messageChanged("请点击一条已存在的直线。");
            }
        } else {
            finishDistance(selectedLineForDist, pos);
            // Reset selection visual
            QPen pen = selectedLineForDist->pen();
            pen.setColor(Qt::red); // Assuming default was red
            pen.setWidth(2);
            selectedLineForDist->setPen(pen);
            
            selectedLineForDist = nullptr;
            emit messageChanged("距离测量完成。");
            setMode(ToolMode::None);
            emit modeChanged(ToolMode::None);
        }
    }
    else {
        QGraphicsScene::mousePressEvent(event);
    }
}

void CanvasScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    updatePreview(event->scenePos());
    QGraphicsScene::mouseMoveEvent(event);
}

// Helper to create arc path
static QPainterPath createArcPath(const QPointF& p1, const QPointF& p2, const QPointF& p3) {
    double x1 = p1.x(), y1 = p1.y();
    double x2 = p2.x(), y2 = p2.y();
    double x3 = p3.x(), y3 = p3.y();

    double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    QPainterPath path;
    if (std::abs(D) < 1e-5) {
        // Collinear, just draw lines
        path.moveTo(p1);
        path.lineTo(p2);
        path.lineTo(p3);
        return path;
    }

    double center_x = ((x1*x1 + y1*y1) * (y2 - y3) + (x2*x2 + y2*y2) * (y3 - y1) + (x3*x3 + y3*y3) * (y1 - y2)) / D;
    double center_y = ((x1*x1 + y1*y1) * (x3 - x2) + (x2*x2 + y2*y2) * (x1 - x3) + (x3*x3 + y3*y3) * (x2 - x1)) / D;
    QPointF center(center_x, center_y);
    double radius = std::sqrt(std::pow(center_x - x1, 2) + std::pow(center_y - y1, 2));

    double startAngle = std::atan2(-(y1 - center_y), x1 - center_x) * 180.0 / PI;
    double midAngle = std::atan2(-(y2 - center_y), x2 - center_x) * 180.0 / PI;
    double endAngle = std::atan2(-(y3 - center_y), x3 - center_x) * 180.0 / PI;

    // Normalize angles to 0-360 for easier calculation logic, but arcTo uses degrees
    // Qt arcTo: startAngle, sweepLength. 
    // We need to determine sweep direction.
    
    // Using cross product to determine orientation of p1->p2->p3
    double cross = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
    bool ccw = cross > 0; // If cross > 0, p3 is to the left of p1->p2

    // Actually, we need the arc from p1 to p3 passing through p2.
    // Calculate angular difference
    
    double sweep = 0;
    
    // Adjust angles to be positive 0-360? No, atan2 returns -180 to 180.
    // Let's just work with diffs.
    
    // Check if we go CW or CCW to hit p2 between p1 and p3
    // Simple approach: check if p2 angle is in the sweep interval
    
    // Let's just try sweep = end - start. 
    // Then normalize to (-360, 360).
    // Then check if mid is in between.
    
    // Robust way:
    // Calculate angle1, angle2, angle3 in [0, 360)
    if (startAngle < 0) startAngle += 360;
    if (midAngle < 0) midAngle += 360;
    if (endAngle < 0) endAngle += 360;

    // Case 1: CCW sweep
    double sweepCCW = endAngle - startAngle;
    if (sweepCCW < 0) sweepCCW += 360;
    
    // Check if mid is in this range
    bool midInCCW = false;
    if (startAngle < endAngle) {
        midInCCW = (midAngle > startAngle && midAngle < endAngle);
    } else { // wrap around
        midInCCW = (midAngle > startAngle || midAngle < endAngle);
    }
    
    if (midInCCW) {
        // CCW is correct
        // Qt arcTo sweep is positive for CCW (wait, Qt documentation says positive is counter-clockwise? 
        // QPainterPath::arcTo: "Positive values for the sweep length mean the arc is drawn counter-clockwise.")
        // Yes.
        // But QGraphicsScene Y is down. So visual CCW might be different.
        // In standard math (Y up), +angle is CCW. In Qt (Y down), +angle is CW visually (clockwise on screen).
        // atan2(y, x) with Y down:
        // y+ is down. 
        // (1, 1) -> 45 deg. Visually bottom-right.
        // (1, -1) -> -45 deg. Visually top-right.
        // So +angle moves from X axis towards Y axis (CW visually).
        // arcTo positive sweep: "counter-clockwise". This usually means decreasing angle in Y-down system?
        // Actually Qt docs say: "0 degrees is at the 3 o'clock position... positive values mean counter-clockwise."
        // In Y-down: 3 o'clock is X+. 12 o'clock is Y-.
        // Rotation from X+(3) to Y-(12) is -90 degrees.
        // So "Counter-Clockwise" in Qt logic means moving towards negative Y.
        
        // Let's stick to the calculated values. atan2 accounts for Y direction.
        // If we use standard logic:
        // sweep = end - start.
        // We just need to handle the "pass through p2" constraint.
        
        // Let's re-eval.
        // We determined midInCCW based on 0-360 logic.
        // If midInCCW is true, we want to sweep CCW (positive direction in standard circle, negative visually in Y-down? No, just "increasing angle" logic works if consistent).
        // Wait, atan2 with Y-down:
        // (1,0) = 0. (0,1) = 90. (-1,0) = 180. (0,-1) = -90.
        // 0 -> 90 is moving Down (CW visually).
        // So increasing angle is CW visually.
        // Qt arcTo "positive sweep means counter-clockwise".
        // This implies Qt's "counter-clockwise" is decreasing angle (0 -> -90 -> -180).
        
        // So if we want to go from Start to End increasing the angle (0 -> 90), that is CW visually.
        // Qt calls this Negative sweep?
        // Let's verify: "Positive values for sweep length mean the arc is drawn counter-clockwise."
        // 0 to 90 (3 o'clock to 6 o'clock) is Clockwise.
        // So sweep should be negative?
        // Let's rely on just using `startAngle` and `sweep` where `sweep` makes us hit `mid`.
        
        // Let's calculate both sweeps:
        double sweep1 = endAngle - startAngle; // Direct
        if (sweep1 > 180) sweep1 -= 360;
        else if (sweep1 < -180) sweep1 += 360;
        
        // This gives shortest path. But we must pass through mid.
        // Check if mid is on this shortest path.
        double diffMidStart = midAngle - startAngle;
        if (diffMidStart > 180) diffMidStart -= 360;
        else if (diffMidStart < -180) diffMidStart += 360;
        
        // If signs match and abs(diffMidStart) < abs(sweep1), then mid is on the way.
        // Or complex logic.
        
        // Simpler:
        // Calculate sweep for (Start -> Mid) and (Mid -> End).
        // Then add them up? No.
        
        // Let's go back to 0-360 logic.
        // We have start, mid, end in [0, 360).
        // CW path: Start -> ... -> End (increasing? No, Y-down increasing is CW).
        // CCW path: Start -> ... -> End (decreasing).
        
        // Path 1 (Increasing / "CW" in Y-down):
        double distCW = endAngle - startAngle;
        if (distCW < 0) distCW += 360;
        // Check if mid is in this interval
        double midRel = midAngle - startAngle;
        if (midRel < 0) midRel += 360;
        bool midInCW = (midRel < distCW);
        
        double finalSweep = 0;
        if (midInCW) {
             // We want the CW path.
             // In Qt arcTo: "positive is CCW". So CW is negative.
             // But wait, in Y-down, increasing angle (0->90) is CW.
             // Does Qt consider 0->90 as positive sweep or negative?
             // Usually angles match. start 0, sweep 90 -> ends at 90.
             // If 90 is at 6 o'clock, then 0->90 is CW.
             // If Qt calls "positive sweep" as CCW, then for Y-down, positive sweep must decrease angle? (0 -> -90).
             // This is confusing. Let's assume:
             // path.arcTo(rect, start, sweep).
             // If I want to go from startAngle to endAngle via midAngle.
             
             // Let's just try constructing it as start->sweep.
             // If we decided CW (increasing angle) is the correct way:
             // Sweep should be -distCW (if Qt logic holds) OR +distCW (if angle logic holds).
             // Let's use -distCW because "positive is CCW".
             finalSweep = -distCW;
        } else {
             // We want the CCW path (decreasing angle).
             // Distance is 360 - distCW.
             double distCCW = 360 - distCW;
             finalSweep = distCCW;
        }
        
        // Override logic: In Qt QGraphicsScene (Y down):
        // Positive angle = Clockwise direction on screen.
        // QPainterPath::arcTo documentation: "Positive values for the sweep length mean the arc is drawn counter-clockwise."
        // This implies positive sweep goes against the increasing angle? i.e. decreases angle.
        // So if we want to go 0 -> 90 (CW), we need sweep = -90.
        // If we want to go 0 -> -90 (CCW), we need sweep = 90.
        
        
        // Note: arcTo expects startAngle in degrees. And standard Qt coord system for arcTo is Y-up? 
        // No, QPainterPath operates in logical coordinates.
        // But: "The angles are specified in degrees. 0 degrees is at the 3 o'clock position... positive values mean counter-clockwise."
        // This definition (Positive=CCW) is consistent with standard math (Y-up).
        // In Y-down, 0 is 3 o'clock. 90 is 6 o'clock.
        // Counter-clockwise from 3 o'clock (0) goes to 12 o'clock (-90? No, Y is negative there).
        // So in Y-down, CCW is moving towards negative Y (negative angles).
        // So 0 -> -90 is CCW.
        // So if we want to draw 0 to -90, sweep is +90.
        // If we want to draw 0 to 90 (CW), sweep is -90.
        
        // My calculation of startAngle uses atan2(y, x).
        // With Y-down, (0,1) is 90.
        // So startAngle is correct in Y-down "degrees".
        // If I want to draw from startAngle to endAngle.
        // If midInCW (increasing angle path):
        // We go from start -> end by INCREASING angle (e.g. 10 -> 20).
        // This is CW visual.
        // Qt requires negative sweep for CW.
        // Sweep = -(end - start).
        // But we normalized distCW to be positive (0 to 360).
        // So finalSweep = -distCW.
        
        // Wait, arcTo takes startAngle. Does it take "screen angle" or "math angle"?
        // It takes degrees.
        // If I say arcTo(..., 45, ...), it starts at 45 deg.
        // In Y-down, 45 deg is 4:30 position (bottom-right).
        // If I use my atan2 result (which gives 45 for bottom-right), it matches.
        // So startAngle is correct.
        
        // However, QPainterPath::arcTo treats the angle argument as...
        // "startAngle: The start angle of the arc."
        // "sweepLength: The sweep length of the arc."
        // Crucially: "Angles are specified in degrees... positive values mean counter-clockwise."
        
        // So if I want to sweep from 10 to 20 (CW visual, increasing Y-down angle):
        // Start: 10.
        // Sweep: -10. (Because +10 would go 10 -> 0).
        
        // So:
        // 1. Calculate start, mid, end in Y-down degrees (atan2).
        // 2. Determine if sequence start->mid->end is Increasing Angle (CW) or Decreasing Angle (CCW).
        //    (Handling 360 wrap).
        // 3. If Increasing (CW): Sweep is negative of the angular distance.
        // 4. If Decreasing (CCW): Sweep is positive of the angular distance.
        
        // Re-check midInCW logic:
        // distCW = end - start (normalized to 0..360).
        // midRel = mid - start (normalized to 0..360).
        // midInCW = (midRel < distCW).
        
        // If midInCW is true, we are moving in "Increasing Angle" direction.
        // Which is CW visually.
        // So Sweep should be -distCW.
        
        // If midInCW is false, we are moving in "Decreasing Angle" direction.
        // We need to go the other way.
        // Distance is (360 - distCW).
        // Sweep should be +(360 - distCW).
        
        if (midInCW) {
            finalSweep = -distCW;
        } else {
            finalSweep = (360 - distCW);
        }
        
        path.moveTo(p1);
        path.arcTo(center_x - radius, center_y - radius, radius*2, radius*2, startAngle, finalSweep);
    }
    
    return path;
}

// Robust arc path that explicitly passes through p1 -> p2 -> p3
static QPainterPath createArcPathThroughMid(const QPointF& p1, const QPointF& p2, const QPointF& p3) {
    double x1 = p1.x(), y1 = p1.y();
    double x2 = p2.x(), y2 = p2.y();
    double x3 = p3.x(), y3 = p3.y();

    double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    QPainterPath path;
    if (std::abs(D) < 1e-5) {
        path.moveTo(p1);
        path.lineTo(p2);
        path.lineTo(p3);
        return path;
    }

    double cx = ((x1*x1 + y1*y1) * (y2 - y3) + (x2*x2 + y2*y2) * (y3 - y1) + (x3*x3 + y3*y3) * (y1 - y2)) / D;
    double cy = ((x1*x1 + y1*y1) * (x3 - x2) + (x2*x2 + y2*y2) * (x1 - x3) + (x3*x3 + y3*y3) * (x2 - x1)) / D;
    double r = std::sqrt((cx - x1)*(cx - x1) + (cy - y1)*(cy - y1));

    auto angleQt = [&](double x, double y){ return std::atan2(-(y - cy), (x - cx)) * 180.0 / PI; };
    auto norm360 = [&](double a){ while (a < 0) a += 360; while (a >= 360) a -= 360; return a; };
    auto ccwDelta = [&](double aFrom, double aTo){ double d = aTo - aFrom; if (d < 0) d += 360; return d; };

    double a1 = norm360(angleQt(x1, y1));
    double a2 = norm360(angleQt(x2, y2));
    double a3 = norm360(angleQt(x3, y3));

    double ccwTotal = ccwDelta(a1, a3);
    double ccwMid = ccwDelta(a1, a2);
    bool midInCCW = ccwMid <= ccwTotal + 1e-9;

    QRectF rect(cx - r, cy - r, r*2, r*2);
    QPainterPath out;
    out.arcMoveTo(rect, a1);
    if (midInCCW) {
        out.arcTo(rect, a1, ccwMid);
        out.arcTo(rect, a2, ccwDelta(a2, a3));
    } else {
        double cw1 = ccwDelta(a2, a1);
        double cw2 = ccwDelta(a3, a2);
        out.arcTo(rect, a1, -cw1);
        out.arcTo(rect, a2, -cw2);
    }
    return out;
}

void CanvasScene::updatePreview(const QPointF& pos) {
    if (currentMode == ToolMode::Line && currentPoints.size() == 1) {
        if (!tempLine) {
            tempLine = addLine(QLineF(currentPoints[0], pos), QPen(Qt::DashLine));
        } else {
            tempLine->setLine(QLineF(currentPoints[0], pos));
        }
    } else if (currentMode == ToolMode::Arc) {
        if (currentPoints.size() == 1) {
            // Preview straight line to second point
             if (!tempArc) {
                // Using path for flexibility
                QPainterPath path;
                path.moveTo(currentPoints[0]);
                path.lineTo(pos);
                tempArc = addPath(path, QPen(Qt::DashLine));
            } else {
                QPainterPath path;
                path.moveTo(currentPoints[0]);
                path.lineTo(pos);
                tempArc->setPath(path);
            }
        } else if (currentPoints.size() == 2) {
             if (!tempArc) {
                 tempArc = addPath(QPainterPath(), QPen(Qt::DashLine));
             }
             
             QPainterPath path = createArcPathThroughMid(currentPoints[0], currentPoints[1], pos);
             tempArc->setPath(path);
        }
    } else if (currentMode == ToolMode::Angle) {
        if (currentPoints.size() == 1) {
             if (!tempLine) {
                tempLine = addLine(QLineF(currentPoints[0], pos), QPen(Qt::DashLine));
            } else {
                tempLine->setLine(QLineF(currentPoints[0], pos));
            }
        } else if (currentPoints.size() == 2) {
            // Show both lines: p1->p2 and p2->pos
            QPainterPath path;
            path.moveTo(currentPoints[0]);
            path.lineTo(currentPoints[1]);
            path.lineTo(pos);
            
            // We need a path item for this, not line item.
            // Reuse tempLine? No, tempLine is QGraphicsLineItem.
            // We need to swap it or use a new item.
            // Simpler: use tempArc (which is QGraphicsPathItem) for Angle mode too?
            // Or just change tempLine to be QGraphicsPathItem generally?
            // Let's just add a new tempPath for Angle if needed, or cast.
            // Easier: Delete tempLine and create tempArc (PathItem) if it's null.
            
            if (tempLine) {
                removeItem(tempLine);
                delete tempLine;
                tempLine = nullptr;
            }
            
            if (!tempArc) {
                 tempArc = addPath(path, QPen(Qt::DashLine));
            } else {
                 tempArc->setPath(path);
            }
        }
    }
}

void CanvasScene::finishLine(const QPointF& p1, const QPointF& p2) {
    QPen pen(Qt::red);
    pen.setWidth(2);
    QGraphicsLineItem* lineItem = addLine(QLineF(p1, p2), pen);

    double lengthPx = dist(p1, p2);
    double lengthMm = toMm(lengthPx);

    QGraphicsSimpleTextItem* text = addSimpleText(QString::number(lengthMm, 'f', 2) + " mm");
    text->setBrush(Qt::blue);
    text->setPos((p1 + p2) / 2);
    
    measureItems.append({0, lineItem, text, {p1, p2}, {}});
}

void CanvasScene::finishArc(const QPointF& p1, const QPointF& p2, const QPointF& p3) {
    double x1 = p1.x(), y1 = p1.y();
    double x2 = p2.x(), y2 = p2.y();
    double x3 = p3.x(), y3 = p3.y();

    double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (std::abs(D) < 1e-5) {
        emit messageChanged("三点共线，无法绘制圆弧");
        return;
    }

    double center_x = ((x1*x1 + y1*y1) * (y2 - y3) + (x2*x2 + y2*y2) * (y3 - y1) + (x3*x3 + y3*y3) * (y1 - y2)) / D;
    double center_y = ((x1*x1 + y1*y1) * (x3 - x2) + (x2*x2 + y2*y2) * (x1 - x3) + (x3*x3 + y3*y3) * (x2 - x1)) / D;

    QPointF center(center_x, center_y);
    double radiusPx = dist(center, p1);
    double radiusMm = toMm(radiusPx);

    // Use the helper to get the correct arc path
    QPainterPath path = createArcPathThroughMid(p1, p2, p3);
    QGraphicsPathItem* arcItem = addPath(path, QPen(Qt::green, 2));

    QList<QGraphicsItem*> extras;
    extras.append(addEllipse(x1-2, y1-2, 4, 4, QPen(Qt::green), QBrush(Qt::green)));
    extras.append(addEllipse(x2-2, y2-2, 4, 4, QPen(Qt::green), QBrush(Qt::green)));
    extras.append(addEllipse(x3-2, y3-2, 4, 4, QPen(Qt::green), QBrush(Qt::green)));

    QGraphicsSimpleTextItem* text = addSimpleText(QString("半径: %1 mm").arg(radiusMm, 0, 'f', 2));
    text->setBrush(Qt::blue);
    text->setPos(center);
    
    measureItems.append({1, arcItem, text, {p1, p2, p3}, extras});
}

void CanvasScene::finishAngle(const QPointF& p1, const QPointF& p2, const QPointF& p3) {
    double v1x = p1.x() - p2.x();
    double v1y = p1.y() - p2.y();
    double v2x = p3.x() - p2.x();
    double v2y = p3.y() - p2.y();

    double dot = v1x * v2x + v1y * v2y;
    double mag1 = std::sqrt(v1x*v1x + v1y*v1y);
    double mag2 = std::sqrt(v2x*v2x + v2y*v2y);
    double angleRad = std::acos(dot / (mag1 * mag2));
    double angleDeg = angleRad * 180.0 / PI;

    QPainterPath path;
    path.moveTo(p2);
    path.lineTo(p1);
    path.moveTo(p2);
    path.lineTo(p3);
    QGraphicsPathItem* item = addPath(path, QPen(Qt::yellow, 2));

    QGraphicsSimpleTextItem* text = addSimpleText(QString("%1°").arg(angleDeg, 0, 'f', 1));
    text->setBrush(Qt::blue);
    text->setPos(p2 + QPointF(10, 10));

    measureItems.append({3, item, text, {p1, p2, p3}, {}});
}

void CanvasScene::finishDistance(QGraphicsLineItem* line, const QPointF& point) {
    QLineF l = line->line();
    
    double lx1 = l.x1();
    double ly1 = l.y1();
    double lx2 = l.x2();
    double ly2 = l.y2();
    double px = point.x();
    double py = point.y();
    
    double ABx = lx2 - lx1;
    double ABy = ly2 - ly1;
    double APx = px - lx1;
    double APy = py - ly1;
    
    double t = (APx * ABx + APy * ABy) / (ABx * ABx + ABy * ABy);
    
    double projX = lx1 + t * ABx;
    double projY = ly1 + t * ABy;
    
    QPointF projPoint(projX, projY);
    
    double distPx = dist(point, projPoint);
    double distMm = toMm(distPx);

    QGraphicsLineItem* distLine = addLine(QLineF(point, projPoint), QPen(Qt::cyan, 1, Qt::DashLine));
    
    QGraphicsSimpleTextItem* text = addSimpleText(QString("距离: %1 mm").arg(distMm, 0, 'f', 2));
    text->setBrush(Qt::blue);
    text->setPos((point + projPoint) / 2);
    
    measureItems.append({2, distLine, text, {point, projPoint}, {}});
}

void CanvasScene::clearMeasurements() {
    for (const auto& item : measureItems) {
        removeItem(item.graphicsItem);
        removeItem(item.textItem);
        for (auto* extra : item.extraItems) {
            removeItem(extra);
            delete extra;
        }
        delete item.graphicsItem;
        delete item.textItem;
    }
    measureItems.clear();
    
    // Clear temp items if any
    setMode(ToolMode::None);
    emit modeChanged(ToolMode::None);
    
    emit messageChanged("所有绘制对象已清除");
}
