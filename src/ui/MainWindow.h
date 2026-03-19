#pragma once
#include <QMainWindow>
#include "document/Document.h"
#include "tools/SketchTool.h"
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QActionGroup;
QT_END_NAMESPACE

namespace elcad {

class ViewportWidget;
class BodyListPanel;
class PropertiesPanel;
class NavCubeWidget;
class Document;
class SketchTool;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupDocks();
    void setupStatusBar();
    void createTestScene();

    void onImportStep();
    void onExportStep();

    // Sketch mode helpers
    void enterSketch(int planeId);
    void exitSketch();
    void activateSketchTool(int toolId);
    void updateToolActions(int activeId);

    // 3D operation helpers
    void onExtrude();
    void onMirror();
    void onBooleanUnion();
    void onBooleanSubtract();

    std::unique_ptr<Document> m_document;

    ViewportWidget*  m_viewport{nullptr};
    BodyListPanel*   m_bodyListPanel{nullptr};
    PropertiesPanel* m_propertiesPanel{nullptr};
    NavCubeWidget*   m_navCube{nullptr};
    QLabel*          m_statusCoords{nullptr};
    QLabel*          m_statusMode{nullptr};
    QLabel*          m_statusSnap{nullptr};

    // Sketch tool actions (kept for checkmark management)
    QAction* m_actLine{nullptr};
    QAction* m_actRect{nullptr};
    QAction* m_actCircle{nullptr};
    QAction* m_actCons{nullptr};
    QAction* m_actExitSketch{nullptr};

    // Gizmo mode toggle actions
    QAction* m_actGizmoTranslate{nullptr};
    QAction* m_actGizmoRotate{nullptr};
    QAction* m_actGizmoScale{nullptr};

    // Undo/Redo actions (kept to update text and enabled state)
    QAction* m_actUndo{nullptr};
    QAction* m_actRedo{nullptr};

    void refreshUndoRedoActions();

    // Owned tools (recreated per sketch)
    std::unique_ptr<SketchTool> m_currentTool;
};

} // namespace elcad
