#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QLabel>
#include <QAction>
#include "CanvasScene.h"
#include "CanvasView.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onImportImage();
    void onSetScale();
    void onModeLine();
    void onModeArc();
    void onModeAngle();
    void onModeDistance();
    void onClear();
    void onModeChanged(ToolMode mode);
    void updateStatus(const QString& message);

private:
    void createActions();
    void createToolbar();
    void resetActions();

    CanvasView* view;
    CanvasScene* scene;
    QLabel* statusLabel;

    QAction* actionImport;
    QAction* actionSetScale;
    QAction* actionLine;
    QAction* actionArc;
    QAction* actionAngle;
    QAction* actionDistance;
    QAction* actionClear;
};

#endif // MAINWINDOW_H
