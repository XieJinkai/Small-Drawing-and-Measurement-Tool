#include "MainWindow.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QActionGroup>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), scene(new CanvasScene(this)) {
    
    view = new CanvasView(scene);
    setCentralWidget(view);
    
    createActions();
    createToolbar();

    statusLabel = new QLabel("就绪");
    statusBar()->addWidget(statusLabel);

    connect(scene, &CanvasScene::messageChanged, this, &MainWindow::updateStatus);
    connect(scene, &CanvasScene::modeChanged, this, &MainWindow::onModeChanged);
    
    resize(1024, 768);
    setWindowTitle("测量工具");
}

MainWindow::~MainWindow() {
}

void MainWindow::createActions() {
    actionImport = new QAction("导入图片", this);
    connect(actionImport, &QAction::triggered, this, &MainWindow::onImportImage);

    actionSetScale = new QAction("设置比例尺", this);
    connect(actionSetScale, &QAction::triggered, this, &MainWindow::onSetScale);

    actionLine = new QAction("直线测量", this);
    actionLine->setCheckable(true);
    connect(actionLine, &QAction::triggered, this, &MainWindow::onModeLine);

    actionArc = new QAction("圆弧测量", this);
    actionArc->setCheckable(true);
    connect(actionArc, &QAction::triggered, this, &MainWindow::onModeArc);

    actionAngle = new QAction("角度测量", this);
    actionAngle->setCheckable(true);
    connect(actionAngle, &QAction::triggered, this, &MainWindow::onModeAngle);

    actionDistance = new QAction("点线距离", this);
    actionDistance->setCheckable(true);
    connect(actionDistance, &QAction::triggered, this, &MainWindow::onModeDistance);
    
    actionClear = new QAction("清除所有", this);
    connect(actionClear, &QAction::triggered, this, &MainWindow::onClear);
    
    // Group for modes
    QActionGroup* modeGroup = new QActionGroup(this);
    modeGroup->addAction(actionLine);
    modeGroup->addAction(actionArc);
    modeGroup->addAction(actionAngle);
    modeGroup->addAction(actionDistance);
    actionLine->setChecked(false);
}

void MainWindow::createToolbar() {
    QToolBar* toolbar = addToolBar("工具栏");
    toolbar->addAction(actionImport);
    toolbar->addAction(actionSetScale);
    toolbar->addSeparator();
    toolbar->addAction(actionLine);
    toolbar->addAction(actionArc);
    toolbar->addAction(actionAngle);
    toolbar->addAction(actionDistance);
    toolbar->addSeparator();
    toolbar->addAction(actionClear);
}

void MainWindow::onImportImage() {
    QString path = QFileDialog::getOpenFileName(this, "打开图片", "", "图片文件 (*.png *.jpg *.jpeg *.bmp)");
    if (!path.isEmpty()) {
        scene->loadImage(path);
        updateStatus("图片已加载: " + path);
    }
}

void MainWindow::onSetScale() {
    bool ok;
    double val = QInputDialog::getDouble(this, "设置比例尺", "输入每1mm对应的像素数:", scene->getScaleRatio(), 0.1, 10000.0, 2, &ok);
    if (ok) {
        scene->setScaleRatio(val);
        updateStatus(QString("比例尺已设置: %1 px/mm").arg(val));
    }
}

void MainWindow::onModeLine() {
    if (actionLine->isChecked()) {
        scene->setMode(ToolMode::Line);
        updateStatus("模式: 直线测量 (点击2个点)");
    }
}

void MainWindow::onModeArc() {
    if (actionArc->isChecked()) {
        scene->setMode(ToolMode::Arc);
        updateStatus("模式: 圆弧测量 (点击3个点)");
    }
}

void MainWindow::onModeAngle() {
    if (actionAngle->isChecked()) {
        scene->setMode(ToolMode::Angle);
        updateStatus("模式: 角度测量 (点击3个点: 起点, 顶点, 终点)");
    }
}

void MainWindow::onModeDistance() {
    if (actionDistance->isChecked()) {
        scene->setMode(ToolMode::Distance);
        updateStatus("模式: 点线距离 (先选中直线, 再点击点)");
    }
}

void MainWindow::onClear() {
    scene->clearMeasurements();
}

void MainWindow::updateStatus(const QString& message) {
    statusLabel->setText(message);
}

void MainWindow::onModeChanged(ToolMode mode) {
    if (mode == ToolMode::None) {
        resetActions();
        updateStatus("就绪");
    }
}

void MainWindow::resetActions() {
    // Uncheck all mode actions
    actionLine->setChecked(false);
    actionArc->setChecked(false);
    actionAngle->setChecked(false);
    actionDistance->setChecked(false);
}
