#include "ui/MainWindow.h"
#include "ui/ViewportWidget.h"
#include "ui/BodyListPanel.h"
#include "ui/PropertiesPanel.h"
#include "ui/ExtrudeDialog.h"
#include "ui/MirrorDialog.h"
#include "ui/BooleanDialog.h"
#include "ui/ShortcutsDialog.h"
#include "core/Logger.h"
#include "viewport/Gizmo.h"
#include "sketch/SketchPlane.h"
#include "sketch/Sketch.h"
#include "sketch/ExtrudeOperation.h"
#include "document/Document.h"
#include "document/Body.h"
#include "document/TransformOps.h"
#include "document/StepIO.h"
#include "document/UndoStack.h"
#include "tools/LineTool.h"
#include "tools/RectTool.h"
#include "tools/CircleTool.h"
#include "ui/NavCubeWidget.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QDockWidget>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QKeySequence>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

#ifdef ELCAD_HAVE_OCCT
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <gp_Ax2.hxx>
#endif

namespace elcad {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_document(std::make_unique<Document>())
{
    LOG_INFO("MainWindow: constructing");
    setWindowTitle("elcad");
    resize(1400, 900);

    m_viewport = new ViewportWidget(this);
    m_viewport->setDocument(m_document.get());
    setCentralWidget(m_viewport);

    setupMenuBar();
    setupToolBar();
    setupDocks();
    setupStatusBar();

    // Wire undo/redo UI refresh to stack changes
    m_document->undoStack().onChange = [this] { refreshUndoRedoActions(); };

    // Wire cursor coordinates to status bar
    connect(m_viewport, &ViewportWidget::cursorWorldPos,
            this, [this](float x, float y, float z) {
        if (!m_viewport->inSketchMode())
            m_statusCoords->setText(
                QString("X: %1  Y: %2  Z: %3 mm")
                    .arg(x, 0, 'f', 2)
                    .arg(y, 0, 'f', 2)
                    .arg(z, 0, 'f', 2));
    });

    connect(m_viewport, &ViewportWidget::sketchCursorPos,
            this, [this](float u, float v) {
        m_statusCoords->setText(
            QString("U: %1  V: %2 mm").arg(u, 0, 'f', 2).arg(v, 0, 'f', 2));
        if (m_viewport->activeTool())
            m_statusMode->setText("Sketch | " + m_viewport->activeTool()->statusHint());
    });

    // Redraw viewport when bodies change
    connect(m_document.get(), &Document::bodyAdded,
            m_viewport, [this](Body*) { m_viewport->update(); });
    connect(m_document.get(), &Document::bodyChanged,
            m_viewport, [this](Body*) { m_viewport->update(); });
    connect(m_document.get(), &Document::bodyRemoved,
            m_viewport, [this](quint64) { m_viewport->update(); });
    connect(m_document.get(), &Document::selectionChanged,
            m_viewport, [this]() { m_viewport->update(); });

    connect(m_document.get(), &Document::activeSketchChanged,
            this, [this](Sketch* sketch) {
        if (sketch) {
            m_viewport->enterSketchMode(sketch);
            m_statusMode->setText("Mode: Sketch");
        } else {
            m_viewport->exitSketchMode();
            m_statusMode->setText("Mode: Select");
            if (m_actExitSketch) m_actExitSketch->setVisible(false);
        }
    });
    connect(m_viewport, &ViewportWidget::requestExitSketch,
            this, &MainWindow::exitSketch);

    createTestScene();
    LOG_INFO("MainWindow: ready");
}

// ── Menu Bar ───────────────────────────────────────────────────────────────────

void MainWindow::refreshUndoRedoActions()
{
    const auto& stack = m_document->undoStack();
    m_actUndo->setEnabled(stack.canUndo());
    m_actUndo->setText(stack.canUndo() ? "&Undo: " + stack.undoText() : "&Undo");
    m_actRedo->setEnabled(stack.canRedo());
    m_actRedo->setText(stack.canRedo() ? "&Redo: " + stack.redoText() : "&Redo");
}

void MainWindow::setupMenuBar()
{
    // File
    auto* file = menuBar()->addMenu("&File");

    auto* actNew = file->addAction("&New");
    actNew->setShortcut(QKeySequence::New);

    file->addSeparator();

    auto* actImport = file->addAction("&Import STEP…");
    actImport->setShortcut(QKeySequence("Ctrl+I"));
    connect(actImport, &QAction::triggered, this, &MainWindow::onImportStep);

    auto* actExport = file->addAction("&Export STEP…");
    actExport->setShortcut(QKeySequence("Ctrl+S"));
    connect(actExport, &QAction::triggered, this, &MainWindow::onExportStep);

    file->addSeparator();
    auto* actQuit = file->addAction("&Quit");
    actQuit->setShortcut(QKeySequence::Quit);
    connect(actQuit, &QAction::triggered, qApp, &QApplication::quit);

    // Edit
    auto* edit = menuBar()->addMenu("&Edit");
    m_actUndo = edit->addAction("&Undo");
    m_actUndo->setShortcut(QKeySequence::Undo);
    m_actUndo->setEnabled(false);
    connect(m_actUndo, &QAction::triggered, this, [this] {
        m_document->undoStack().undo();
        m_viewport->update();
    });
    m_actRedo = edit->addAction("&Redo");
    m_actRedo->setShortcut(QKeySequence::Redo);
    m_actRedo->setEnabled(false);
    connect(m_actRedo, &QAction::triggered, this, [this] {
        m_document->undoStack().redo();
        m_viewport->update();
    });

    // View
    auto* view = menuBar()->addMenu("&View");
    auto* actGrid = view->addAction("Toggle &Grid");
    actGrid->setShortcut(Qt::Key_G);
    connect(actGrid, &QAction::triggered, this, [this] {
        m_viewport->renderer().setGridVisible(!m_viewport->renderer().gridVisible());
        m_viewport->update();
    });

    auto* actSnap = view->addAction("Toggle &Snap");
    actSnap->setShortcut(QKeySequence("Shift+G"));
    connect(actSnap, &QAction::triggered, this, [this] {
        auto& se = m_viewport->snapEngine();
        se.setSnapEnabled(!se.snapEnabled());
        m_viewport->update();
    });

    auto* actFit = view->addAction("&Fit All");
    actFit->setShortcut(Qt::Key_F);
    connect(actFit, &QAction::triggered, this, [this] {
        m_viewport->camera().fitAll();
        m_viewport->update();
    });

    view->addSeparator();
    auto* actPersp = view->addAction("&Perspective / Ortho");
    actPersp->setShortcut(Qt::Key_P);
    connect(actPersp, &QAction::triggered, this, [this] {
        m_viewport->camera().setPerspective(!m_viewport->camera().isPerspective());
        m_viewport->update();
    });

    view->addSeparator();
    auto* viewMenu = view->addMenu("Standard &Views");
    auto* actFront = viewMenu->addAction("&Front  [Num 1]");
    connect(actFront, &QAction::triggered, this, [this] { m_viewport->camera().setViewFront();     m_viewport->update(); });
    auto* actRight = viewMenu->addAction("&Right  [Num 3]");
    connect(actRight, &QAction::triggered, this, [this] { m_viewport->camera().setViewRight();     m_viewport->update(); });
    auto* actTop   = viewMenu->addAction("&Top    [Num 7]");
    connect(actTop,   &QAction::triggered, this, [this] { m_viewport->camera().setViewTop();       m_viewport->update(); });
    auto* actIso   = viewMenu->addAction("&Isometric [Num 0]");
    connect(actIso,   &QAction::triggered, this, [this] { m_viewport->camera().setViewIsometric(); m_viewport->update(); });

    view->addSeparator();
    auto* actShortcuts = view->addAction("&Keyboard Shortcuts...");
    actShortcuts->setShortcut(Qt::Key_Question);
    connect(actShortcuts, &QAction::triggered, this, [this] {
        ShortcutsDialog dlg(this);
        dlg.exec();
    });
}

// ── Tool Bar ───────────────────────────────────────────────────────────────────

void MainWindow::setupToolBar()
{
    auto* tb = addToolBar("Tools");
    tb->setMovable(false);
    tb->setIconSize(QSize(20, 20));

    // ── Sketch entry ─────────────────────────────────────────────────────────
    tb->addWidget(new QLabel("  Sketch: "));

    auto* actSketchXZ = tb->addAction("XZ Plane");
    actSketchXZ->setToolTip("Enter sketch on XZ plane (front)");
    connect(actSketchXZ, &QAction::triggered, this, [this]{ enterSketch(0); });

    auto* actSketchXY = tb->addAction("XY Plane");
    actSketchXY->setToolTip("Enter sketch on XY plane (ground)");
    connect(actSketchXY, &QAction::triggered, this, [this]{ enterSketch(1); });

    auto* actSketchYZ = tb->addAction("YZ Plane");
    actSketchYZ->setToolTip("Enter sketch on YZ plane (side)");
    connect(actSketchYZ, &QAction::triggered, this, [this]{ enterSketch(2); });

    tb->addSeparator();

    // ── Sketch drawing tools (visible only while in sketch mode) ─────────────
    m_actLine = tb->addAction("Line");
    m_actLine->setCheckable(true);
    m_actLine->setToolTip("Line tool (chained)");
    m_actLine->setVisible(false);
    connect(m_actLine, &QAction::triggered, this, [this]{ activateSketchTool(1); });

    m_actRect = tb->addAction("Rect");
    m_actRect->setCheckable(true);
    m_actRect->setToolTip("Rectangle tool");
    m_actRect->setVisible(false);
    connect(m_actRect, &QAction::triggered, this, [this]{ activateSketchTool(2); });

    m_actCircle = tb->addAction("Circle");
    m_actCircle->setCheckable(true);
    m_actCircle->setToolTip("Circle tool");
    m_actCircle->setVisible(false);
    connect(m_actCircle, &QAction::triggered, this, [this]{ activateSketchTool(3); });

    m_actCons = tb->addAction("Constr.");
    m_actCons->setCheckable(true);
    m_actCons->setToolTip("Construction line tool");
    m_actCons->setVisible(false);
    connect(m_actCons, &QAction::triggered, this, [this]{ activateSketchTool(4); });

    m_actExitSketch = tb->addAction("⏎ Exit Sketch");
    m_actExitSketch->setToolTip("Finish sketch and return to 3D mode");
    m_actExitSketch->setVisible(false);
    connect(m_actExitSketch, &QAction::triggered, this, &MainWindow::exitSketch);

    tb->addSeparator();

    // ── Gizmo mode (visible only in 3D mode, not sketch mode) ───────────────
    tb->addWidget(new QLabel("  Gizmo: "));

    m_actGizmoTranslate = tb->addAction("Move [T]");
    m_actGizmoTranslate->setCheckable(true);
    m_actGizmoTranslate->setChecked(true);
    m_actGizmoTranslate->setToolTip("Translate gizmo (T)");

    m_actGizmoRotate = tb->addAction("Rotate [R]");
    m_actGizmoRotate->setCheckable(true);
    m_actGizmoRotate->setToolTip("Rotate gizmo (R)");

    m_actGizmoScale = tb->addAction("Scale [S]");
    m_actGizmoScale->setCheckable(true);
    m_actGizmoScale->setToolTip("Scale gizmo (S)");

    auto syncGizmoActions = [this](GizmoMode mode) {
        m_actGizmoTranslate->setChecked(mode == GizmoMode::Translate);
        m_actGizmoRotate->setChecked   (mode == GizmoMode::Rotate);
        m_actGizmoScale->setChecked    (mode == GizmoMode::Scale);
        m_viewport->setGizmoMode(mode);
    };

    connect(m_actGizmoTranslate, &QAction::triggered, this,
            [=]{ syncGizmoActions(GizmoMode::Translate); });
    connect(m_actGizmoRotate, &QAction::triggered, this,
            [=]{ syncGizmoActions(GizmoMode::Rotate); });
    connect(m_actGizmoScale, &QAction::triggered, this,
            [=]{ syncGizmoActions(GizmoMode::Scale); });

    tb->addSeparator();
    tb->addWidget(new QLabel("  3D: "));
    auto* actExtrude  = tb->addAction("Extrude");
    auto* actMirror   = tb->addAction("Mirror");

    tb->addSeparator();
    tb->addWidget(new QLabel(" Bool: "));
    auto* actUnion    = tb->addAction("Union");
    auto* actSubtract = tb->addAction("Subtract");

    connect(actExtrude,  &QAction::triggered, this, &MainWindow::onExtrude);
    connect(actMirror,   &QAction::triggered, this, &MainWindow::onMirror);
    connect(actUnion,    &QAction::triggered, this, &MainWindow::onBooleanUnion);
    connect(actSubtract, &QAction::triggered, this, &MainWindow::onBooleanSubtract);

    Q_UNUSED(actExtrude) Q_UNUSED(actMirror) Q_UNUSED(actUnion) Q_UNUSED(actSubtract)
}

// ── Docks ─────────────────────────────────────────────────────────────────────

void MainWindow::setupDocks()
{
    // Left dock — body list
    auto* leftDock = new QDockWidget("Bodies", this);
    leftDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    leftDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    m_bodyListPanel = new BodyListPanel(leftDock);
    m_bodyListPanel->setDocument(m_document.get());
    m_bodyListPanel->setMinimumWidth(180);
    leftDock->setWidget(m_bodyListPanel);
    addDockWidget(Qt::LeftDockWidgetArea, leftDock);

    // Right dock — NavCube (top) + properties (bottom)
    auto* rightDock = new QDockWidget("NavCube", this);
    rightDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    rightDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* rightContainer = new QWidget(rightDock);
    auto* rightLayout    = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_navCube = new NavCubeWidget(rightContainer);
    // Centre the NavCube horizontally
    auto* navRow = new QHBoxLayout;
    navRow->addStretch();
    navRow->addWidget(m_navCube);
    navRow->addStretch();
    rightLayout->addLayout(navRow);

    // Thin separator
    auto* sep = new QFrame(rightContainer);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    rightLayout->addWidget(sep);

    // Properties panel label + widget
    auto* propLabel = new QLabel("Properties", rightContainer);
    propLabel->setStyleSheet("font-weight: bold; padding: 4px 6px;");
    rightLayout->addWidget(propLabel);

    m_propertiesPanel = new PropertiesPanel(rightContainer);
    m_propertiesPanel->setDocument(m_document.get());
    m_propertiesPanel->setMinimumWidth(200);
    rightLayout->addWidget(m_propertiesPanel, 1);

    rightDock->setWidget(rightContainer);
    addDockWidget(Qt::RightDockWidgetArea, rightDock);

    // ── Wire NavCube ────────────────────────────────────────────────────────
    connect(m_viewport, &ViewportWidget::cameraOrientationChanged,
            m_navCube,  &NavCubeWidget::setOrientation);

    connect(m_navCube, &NavCubeWidget::orbitRequested,
            m_viewport, &ViewportWidget::applyOrbit);

    connect(m_navCube, &NavCubeWidget::viewPresetRequested,
            m_viewport, &ViewportWidget::applyViewPreset);

    connect(m_navCube, &NavCubeWidget::projectionToggleRequested,
            this, [this] {
        Camera& cam = m_viewport->camera();
        cam.setPerspective(!cam.isPerspective());
        m_viewport->update();
        emit m_viewport->cameraOrientationChanged(cam.yaw(), cam.pitch(), cam.isPerspective());
    });

    // Seed NavCube with the initial camera orientation
    Camera& cam = m_viewport->camera();
    m_navCube->setOrientation(cam.yaw(), cam.pitch(), cam.isPerspective());
}

// ── Test scene ────────────────────────────────────────────────────────────────

void MainWindow::createTestScene()
{
#ifdef ELCAD_HAVE_OCCT
    // Box 100×60×40 mm
    {
        Body* b = m_document->addBody("Box 100×60×40");
        b->setShape(BRepPrimAPI_MakeBox(100.0, 60.0, 40.0).Shape());
        b->setColor(QColor(100, 160, 220));
    }
    // Cylinder r=25, h=80 mm, offset
    {
        Body* b = m_document->addBody("Cylinder r25 h80");
        gp_Ax2 ax(gp_Pnt(150, 0, 0), gp_Dir(0, 0, 1));
        b->setShape(BRepPrimAPI_MakeCylinder(ax, 25.0, 80.0).Shape());
        b->setColor(QColor(220, 140, 80));
    }
    // Sphere r=30 mm
    {
        Body* b = m_document->addBody("Sphere r30");
        gp_Ax2 ax(gp_Pnt(-80, 0, 30), gp_Dir(0, 0, 1));
        b->setShape(BRepPrimAPI_MakeSphere(ax, 30.0).Shape());
        b->setColor(QColor(120, 200, 120));
    }
#endif
}

// ── Status Bar ────────────────────────────────────────────────────────────────

void MainWindow::setupStatusBar()
{
    m_statusMode   = new QLabel("Mode: Select");
    m_statusSnap   = new QLabel("Snap: Grid");
    m_statusCoords = new QLabel("X: 0.00  Y: 0.00  Z: 0.00 mm");

    statusBar()->addWidget(m_statusMode);
    statusBar()->addWidget(new QLabel(" | "));
    statusBar()->addWidget(m_statusSnap);
    statusBar()->addPermanentWidget(m_statusCoords);
}

// ── Sketch helpers ────────────────────────────────────────────────────────────

void MainWindow::enterSketch(int planeId)
{
    if (m_viewport->inSketchMode())
        exitSketch();

    SketchPlane plane;
    switch (planeId) {
    case 0: plane = SketchPlane::xz(); break;
    case 1: plane = SketchPlane::xy(); break;
    case 2: plane = SketchPlane::yz(); break;
    default: plane = SketchPlane::xz(); break;
    }

    LOG_INFO("MainWindow: entering sketch mode on plane '{}'", plane.name().toStdString());
    m_document->beginSketch(plane);  // triggers activeSketchChanged → enterSketchMode

    // Show sketch tool buttons
    for (auto* a : {m_actLine, m_actRect, m_actCircle, m_actCons, m_actExitSketch})
        if (a) a->setVisible(true);

    m_statusSnap->setText(QString("Plane: %1").arg(plane.name()));
    activateSketchTool(1);  // activate Line tool by default
}

void MainWindow::exitSketch()
{
    LOG_INFO("MainWindow: exiting sketch mode");
    m_currentTool.reset();
    m_viewport->setActiveTool(nullptr);
    m_document->endSketch();  // triggers activeSketchChanged(nullptr) → exitSketchMode

    for (auto* a : {m_actLine, m_actRect, m_actCircle, m_actCons, m_actExitSketch})
        if (a) a->setVisible(false);

    m_statusSnap->setText("Snap: Grid");
}

void MainWindow::activateSketchTool(int toolId)
{
    if (!m_viewport->inSketchMode()) return;

    Sketch* sketch = m_document->activeSketch();
    if (!sketch) return;

    m_currentTool.reset();

    switch (toolId) {
    case 1: m_currentTool = std::make_unique<LineTool>(sketch);          break;
    case 2: m_currentTool = std::make_unique<RectTool>(sketch);          break;
    case 3: m_currentTool = std::make_unique<CircleTool>(sketch);        break;
    case 4: m_currentTool = std::make_unique<LineTool>(sketch, true);    break;  // construction
    default: break;
    }

    m_viewport->setActiveTool(m_currentTool.get());
    updateToolActions(toolId);

    if (m_currentTool)
        m_statusMode->setText("Sketch | " + m_currentTool->statusHint());
}

void MainWindow::updateToolActions(int activeId)
{
    struct { QAction* a; int id; } items[] = {
        {m_actLine, 1}, {m_actRect, 2}, {m_actCircle, 3}, {m_actCons, 4}
    };
    for (auto& item : items)
        if (item.a) item.a->setChecked(item.id == activeId);
}

// ── 3D Operations ─────────────────────────────────────────────────────────────

void MainWindow::onExtrude()
{
#ifdef ELCAD_HAVE_OCCT
    // Need an active sketch with geometry
    Sketch* sketch = m_document->activeSketch();
    if (!sketch) {
        // Try last completed sketch
        if (m_document->sketches().empty()) {
            LOG_WARN("Extrude: no sketch available — user notified");
            QMessageBox::warning(this, "Extrude", "No sketch available.\nCreate a sketch first.");
            return;
        }
        sketch = m_document->sketches().back().get();
    }

    ExtrudeDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) {
        LOG_DEBUG("Extrude: dialog cancelled");
        return;
    }

    ExtrudeParams params = dlg.params();
    LOG_INFO("Extrude: distance={:.4f} mode={} symmetric={}",
             params.distance, params.mode, params.symmetric);
    ExtrudeResult res = ExtrudeOperation::extrude(*sketch, params);

    if (!res.success) {
        LOG_ERROR("Extrude failed: {}", res.errorMsg.toStdString());
        QMessageBox::critical(this, "Extrude Failed", res.errorMsg);
        return;
    }

    Body* selectedBody = m_document->singleSelectedBody();

    if (params.mode == 0 || !selectedBody || !selectedBody->hasShape()) {
        // New body — already added; redo just re-inserts it after an undo
        Body* b = m_document->addBody("Extrusion");
        b->setShape(res.shape);
        quint64 bodyId = b->id();
        auto bodyHolder = std::make_shared<std::unique_ptr<Body>>();
        bool firstRedo = true;
        m_document->undoStack().push(std::make_unique<LambdaCommand>(
            "Extrude New",
            [this, bodyId, bodyHolder]() {
                *bodyHolder = m_document->removeBodyRetain(bodyId);
            },
            [this, bodyHolder, firstRedo]() mutable {
                if (firstRedo) { firstRedo = false; return; } // already in document
                m_document->reinsertBody(std::move(*bodyHolder));
            }));
    } else {
        // Add or Remove using boolean
        TopoDS_Shape oldShape = selectedBody->shape();
        ExtrudeResult boolRes;
        if (params.mode == 1)
            boolRes = ExtrudeOperation::booleanAdd(oldShape, res.shape);
        else
            boolRes = ExtrudeOperation::booleanCut(oldShape, res.shape);

        if (!boolRes.success) {
            LOG_ERROR("Extrude boolean (mode={}) failed: {}",
                      params.mode, boolRes.errorMsg.toStdString());
            QMessageBox::critical(this, "Boolean Failed", boolRes.errorMsg);
            return;
        }

        TopoDS_Shape newShape = boolRes.shape;
        m_document->undoStack().push(std::make_unique<LambdaCommand>(
            params.mode == 1 ? "Extrude Add" : "Extrude Remove",
            [this, id = selectedBody->id(), oldShape]() {
                if (Body* b = m_document->bodyById(id)) {
                    b->setShape(oldShape);
                    m_viewport->renderer().invalidateMesh(id);
                    emit m_document->bodyChanged(b);
                }
            },
            [this, id = selectedBody->id(), newShape]() {
                if (Body* b = m_document->bodyById(id)) {
                    b->setShape(newShape);
                    m_viewport->renderer().invalidateMesh(id);
                    emit m_document->bodyChanged(b);
                }
            }));

        selectedBody->setShape(newShape);
        m_viewport->renderer().invalidateMesh(selectedBody->id());
        emit m_document->bodyChanged(selectedBody);
    }

    if (m_document->activeSketch())
        exitSketch();
#else
    QMessageBox::information(this, "Extrude", "OCCT not available.");
#endif
}

void MainWindow::onMirror()
{
#ifdef ELCAD_HAVE_OCCT
    Body* body = m_document->singleSelectedBody();
    if (!body || !body->hasShape()) {
        LOG_WARN("Mirror: no single body selected");
        QMessageBox::warning(this, "Mirror", "Select exactly one body first."); return;
    }
    MirrorDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) { LOG_DEBUG("Mirror: dialog cancelled"); return; }

    LOG_INFO("Mirror: body id={} name='{}' plane={}",
             body->id(), body->name().toStdString(), dlg.mirrorPlane());

    TopoDS_Shape oldShape = body->shape();
    TopoDS_Shape newShape = TransformOps::mirror(oldShape, dlg.mirrorPlane());

    quint64 id = body->id();
    m_document->undoStack().push(std::make_unique<LambdaCommand>("Mirror",
        [this, id, oldShape]() {
            if (Body* b = m_document->bodyById(id)) {
                b->setShape(oldShape); m_viewport->renderer().invalidateMesh(id);
                emit m_document->bodyChanged(b);
            }
        },
        [this, id, newShape]() {
            if (Body* b = m_document->bodyById(id)) {
                b->setShape(newShape); m_viewport->renderer().invalidateMesh(id);
                emit m_document->bodyChanged(b);
            }
        }));

    body->setShape(newShape);
    m_viewport->renderer().invalidateMesh(id);
    emit m_document->bodyChanged(body);
#endif
}

void MainWindow::onBooleanUnion()
{
#ifdef ELCAD_HAVE_OCCT
    if (m_document->bodyCount() < 2) {
        LOG_WARN("Boolean union: need at least 2 bodies, have {}",
                 m_document->bodyCount());
        QMessageBox::warning(this, "Boolean Union", "Need at least 2 bodies."); return;
    }
    BooleanDialog dlg(m_document.get(), BooleanDialog::Union, this);
    if (dlg.exec() != QDialog::Accepted) { LOG_DEBUG("Boolean union: dialog cancelled"); return; }

    Body* target = m_document->bodyById(dlg.targetBodyId());
    Body* tool   = m_document->bodyById(dlg.toolBodyId());
    if (!target || !tool || target == tool) {
        LOG_ERROR("Boolean union: invalid body selection — target id={} tool id={} "
                  "(same={}, null target={}, null tool={})",
                  dlg.targetBodyId(), dlg.toolBodyId(),
                  target == tool, target == nullptr, tool == nullptr);
        QMessageBox::warning(this, "Boolean Union", "Invalid body selection."); return;
    }

    LOG_INFO("Boolean union: target id={} name='{}' + tool id={} name='{}'",
             target->id(), target->name().toStdString(),
             tool->id(), tool->name().toStdString());

    ExtrudeResult res = ExtrudeOperation::booleanAdd(target->shape(), tool->shape());
    if (!res.success) {
        LOG_ERROR("Boolean union failed: {}", res.errorMsg.toStdString());
        QMessageBox::critical(this, "Union Failed", res.errorMsg); return;
    }

    TopoDS_Shape oldTargetShape = target->shape();
    TopoDS_Shape newTargetShape = res.shape;
    quint64 targetId = target->id();
    quint64 toolId   = tool->id();

    target->setShape(newTargetShape);
    m_viewport->renderer().invalidateMesh(targetId);
    emit m_document->bodyChanged(target);
    auto toolHolder = std::make_shared<std::unique_ptr<Body>>(
        m_document->removeBodyRetain(toolId));

    bool firstRedo = true;
    m_document->undoStack().push(std::make_unique<LambdaCommand>(
        "Boolean Union",
        [this, targetId, oldTargetShape, toolHolder]() {
            if (Body* b = m_document->bodyById(targetId)) {
                b->setShape(oldTargetShape);
                m_viewport->renderer().invalidateMesh(targetId);
                emit m_document->bodyChanged(b);
            }
            m_document->reinsertBody(std::move(*toolHolder));
        },
        [this, targetId, newTargetShape, toolHolder, toolId, firstRedo]() mutable {
            if (firstRedo) { firstRedo = false; return; }
            if (Body* b = m_document->bodyById(targetId)) {
                b->setShape(newTargetShape);
                m_viewport->renderer().invalidateMesh(targetId);
                emit m_document->bodyChanged(b);
            }
            *toolHolder = m_document->removeBodyRetain(toolId);
        }));
#endif
}

void MainWindow::onBooleanSubtract()
{
#ifdef ELCAD_HAVE_OCCT
    if (m_document->bodyCount() < 2) {
        LOG_WARN("Boolean subtract: need at least 2 bodies, have {}",
                 m_document->bodyCount());
        QMessageBox::warning(this, "Boolean Subtract", "Need at least 2 bodies."); return;
    }
    BooleanDialog dlg(m_document.get(), BooleanDialog::Subtract, this);
    if (dlg.exec() != QDialog::Accepted) { LOG_DEBUG("Boolean subtract: dialog cancelled"); return; }

    Body* target = m_document->bodyById(dlg.targetBodyId());
    Body* tool   = m_document->bodyById(dlg.toolBodyId());
    if (!target || !tool || target == tool) {
        LOG_ERROR("Boolean subtract: invalid body selection — target id={} tool id={}",
                  dlg.targetBodyId(), dlg.toolBodyId());
        QMessageBox::warning(this, "Boolean Subtract", "Invalid body selection."); return;
    }

    LOG_INFO("Boolean subtract: base id={} name='{}' - tool id={} name='{}'",
             target->id(), target->name().toStdString(),
             tool->id(), tool->name().toStdString());

    ExtrudeResult res = ExtrudeOperation::booleanCut(target->shape(), tool->shape());
    if (!res.success) {
        LOG_ERROR("Boolean subtract failed: {}", res.errorMsg.toStdString());
        QMessageBox::critical(this, "Subtract Failed", res.errorMsg); return;
    }

    TopoDS_Shape oldTargetShape = target->shape();
    TopoDS_Shape newTargetShape = res.shape;
    quint64 targetId = target->id();
    quint64 toolId   = tool->id();

    target->setShape(newTargetShape);
    m_viewport->renderer().invalidateMesh(targetId);
    emit m_document->bodyChanged(target);
    auto toolHolder = std::make_shared<std::unique_ptr<Body>>(
        m_document->removeBodyRetain(toolId));

    bool firstRedo = true;
    m_document->undoStack().push(std::make_unique<LambdaCommand>(
        "Boolean Subtract",
        [this, targetId, oldTargetShape, toolHolder]() {
            if (Body* b = m_document->bodyById(targetId)) {
                b->setShape(oldTargetShape);
                m_viewport->renderer().invalidateMesh(targetId);
                emit m_document->bodyChanged(b);
            }
            m_document->reinsertBody(std::move(*toolHolder));
        },
        [this, targetId, newTargetShape, toolHolder, toolId, firstRedo]() mutable {
            if (firstRedo) { firstRedo = false; return; }
            if (Body* b = m_document->bodyById(targetId)) {
                b->setShape(newTargetShape);
                m_viewport->renderer().invalidateMesh(targetId);
                emit m_document->bodyChanged(b);
            }
            *toolHolder = m_document->removeBodyRetain(toolId);
        }));
#endif
}

// ── File I/O ──────────────────────────────────────────────────────────────────

void MainWindow::onImportStep()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Import STEP", QString(), "STEP Files (*.step *.stp *.STEP *.STP)");
    if (path.isEmpty()) { LOG_DEBUG("Import STEP: dialog cancelled"); return; }

#ifdef ELCAD_HAVE_OCCT
    LOG_INFO("Import STEP: user selected '{}'", path.toStdString());
    StepImportResult res = StepIO::importStep(path);
    if (!res.success) {
        LOG_ERROR("Import STEP failed: {} — file='{}'",
                  res.errorMsg.toStdString(), path.toStdString());
        QMessageBox::critical(this, "Import Failed", res.errorMsg);
        return;
    }

    m_document->clearSelection();
    for (int i = 0; i < res.shapes.size(); ++i) {
        Body* b = m_document->addBody(res.names.value(i, QString("Import_%1").arg(i+1)));
        b->setShape(res.shapes[i]);
        b->setSelected(true);
    }
    emit m_document->selectionChanged();
    LOG_INFO("Import STEP: {} shape(s) added to document from '{}'",
             res.shapes.size(), path.toStdString());

    QMessageBox::information(this, "Import STEP",
        QString("Imported %1 shape(s) from:\n%2").arg(res.shapes.size()).arg(path));
#else
    QMessageBox::information(this, "Import STEP", "OCCT not available.");
#endif
}

void MainWindow::onExportStep()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Export STEP", "model.step", "STEP Files (*.step *.stp)");
    if (path.isEmpty()) { LOG_DEBUG("Export STEP: dialog cancelled"); return; }

#ifdef ELCAD_HAVE_OCCT
    QList<TopoDS_Shape> shapes;
    QList<QString>      names;

    for (auto& b : m_document->bodies()) {
        if (b->visible() && b->hasShape()) {
            shapes.append(b->shape());
            names.append(b->name());
        }
    }

    if (shapes.isEmpty()) {
        LOG_WARN("Export STEP: no visible bodies to export — file='{}'", path.toStdString());
        QMessageBox::warning(this, "Export STEP", "No visible bodies to export.");
        return;
    }

    LOG_INFO("Export STEP: exporting {} body/bodies to '{}'",
             shapes.size(), path.toStdString());
    StepExportResult res = StepIO::exportStep(path, shapes, names);
    if (!res.success) {
        LOG_ERROR("Export STEP failed: {} — file='{}'",
                  res.errorMsg.toStdString(), path.toStdString());
        QMessageBox::critical(this, "Export Failed", res.errorMsg);
    } else {
        LOG_INFO("Export STEP succeeded: {} body/bodies written to '{}'",
                 shapes.size(), path.toStdString());
        QMessageBox::information(this, "Export STEP",
            QString("Exported %1 body/bodies to:\n%2").arg(shapes.size()).arg(path));
    }
#else
    QMessageBox::information(this, "Export STEP", "OCCT not available.");
#endif
}

} // namespace elcad
