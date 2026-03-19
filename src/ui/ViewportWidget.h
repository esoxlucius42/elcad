#pragma once
#include "viewport/Camera.h"
#include "viewport/Gizmo.h"
#include "viewport/Renderer.h"
#include "sketch/SnapEngine.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QPoint>
#include <memory>

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Shape.hxx>
#endif

QT_BEGIN_NAMESPACE
class QRubberBand;
QT_END_NAMESPACE

namespace elcad {

class Document;
class Sketch;
class SketchTool;

class ViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit ViewportWidget(QWidget* parent = nullptr);

    Camera&   camera()   { return m_camera; }
    Renderer& renderer() { return m_renderer; }

    void setDocument(Document* doc);

    // Sketch mode
    void enterSketchMode(Sketch* sketch);
    void exitSketchMode();
    bool inSketchMode() const { return m_sketch != nullptr; }

    void setActiveTool(SketchTool* tool);  // does NOT take ownership
    SketchTool* activeTool() const { return m_activeTool; }

    SnapEngine& snapEngine() { return m_snapEngine; }

    void setGizmoMode(GizmoMode mode);

signals:
    void cursorWorldPos(float x, float y, float z);
    void sketchCursorPos(float u, float v);  // sketch-plane 2D coords
    void requestExitSketch();                // emitted when Esc pressed with no active tool op
    void cameraOrientationChanged(float yaw, float pitch, bool perspective);

public slots:
    void applyOrbit(float dyaw, float dpitch);
    void applyViewPreset(float yaw, float pitch);

protected:
    void initializeGL()         override;
    void resizeGL(int w, int h) override;
    void paintGL()              override;

    void mousePressEvent  (QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent   (QMouseEvent* e) override;
    void wheelEvent       (QWheelEvent* e) override;
    void keyPressEvent    (QKeyEvent*   e) override;

private:
    void handlePickClick(QPoint pos, bool addToSelection);
    void handleBoxSelect(QRect rect, bool addToSelection);
    void commitGizmoDrag();
    QVector2D screenToSketch(QPoint screenPos) const;

    Camera      m_camera;
    Renderer    m_renderer;
    SnapEngine  m_snapEngine;
    Document*   m_document{nullptr};
    Sketch*     m_sketch{nullptr};     // not owned; non-null → sketch mode
    SketchTool* m_activeTool{nullptr}; // not owned

    enum class DragMode { None, Orbit, Pan, BoxSelect, GizmoDrag };
    DragMode m_dragMode{DragMode::None};

    // LMB tracking
    QPoint       m_lmbPressPos;
    bool         m_lmbDown{false};
    QRubberBand* m_rubberBand{nullptr};

    // Last snapped sketch position (for rendering crosshair)
    QVector2D    m_snapPos;
    bool         m_hasSnapPos{false};

    // Gizmo drag state
    Body*        m_gizmoDragBody{nullptr};   // non-owned; valid only during drag
#ifdef ELCAD_HAVE_OCCT
    TopoDS_Shape m_gizmoDragStartShape;      // snapshot at drag start for undo
#endif
};

} // namespace elcad
