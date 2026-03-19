#include "ui/ViewportWidget.h"
#include "core/Logger.h"
#include "document/Document.h"
#include "document/Body.h"
#include "document/TransformOps.h"
#include "document/UndoStack.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"
#include "tools/SketchTool.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QRubberBand>
#include <QSurfaceFormat>

namespace elcad {

static constexpr int kBoxSelectThreshold = 5; // pixels

ViewportWidget::ViewportWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::NoProfile);
    fmt.setSamples(4);
    fmt.setDepthBufferSize(24);
    setFormat(fmt);

    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
}

void ViewportWidget::initializeGL()
{
    initializeOpenGLFunctions();
    m_renderer.initialize();

    const char* glVersion  = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glVendor   = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    LOG_INFO("OpenGL context initialised — version='{}' vendor='{}' renderer='{}'",
             glVersion  ? glVersion  : "?",
             glVendor   ? glVendor   : "?",
             glRenderer ? glRenderer : "?");
}

void ViewportWidget::resizeGL(int w, int h)
{
    m_camera.setViewportSize(w, h);
    m_renderer.resize(w, h);
    LOG_DEBUG("Viewport resized to {}x{}", w, h);
}

void ViewportWidget::paintGL()
{
    std::vector<SketchEntity> preview;
    if (m_activeTool)
        preview = m_activeTool->previewEntities();

    const QVector2D* snapPtr = m_hasSnapPos ? &m_snapPos : nullptr;
    m_renderer.render(m_camera, m_document,
                      preview.empty() ? nullptr : &preview,
                      snapPtr);
}

void ViewportWidget::setDocument(Document* doc)
{
    m_document = doc;
}

void ViewportWidget::setGizmoMode(GizmoMode mode)
{
    m_renderer.gizmo().setMode(mode);
    update();
}

void ViewportWidget::enterSketchMode(Sketch* sketch)
{
    m_sketch      = sketch;
    m_activeTool  = nullptr;
    m_hasSnapPos  = false;
    m_renderer.setActiveSketch(sketch);
    update();
}

void ViewportWidget::exitSketchMode()
{
    m_sketch      = nullptr;
    m_activeTool  = nullptr;
    m_hasSnapPos  = false;
    m_renderer.setActiveSketch(nullptr);
    update();
}

void ViewportWidget::setActiveTool(SketchTool* tool)
{
    m_activeTool = tool;
    if (tool)
        connect(tool, &SketchTool::requestRedraw, this, [this]{ update(); });
    update();
}

// ── Screen → sketch plane conversion ─────────────────────────────────────────

QVector2D ViewportWidget::screenToSketch(QPoint screenPos) const
{
    if (!m_sketch) return {};
    QVector3D ro, rd;
    m_camera.unprojectRay(screenPos.x(), screenPos.y(), width(), height(), ro, rd);
    float t = m_sketch->plane().intersectRay(ro, rd);
    if (t < 0.f) return {};
    QVector3D hit = ro + rd * t;
    return m_sketch->plane().to2D(hit);
}

// ── Mouse ──────────────────────────────────────────────────────────────────────

void ViewportWidget::mousePressEvent(QMouseEvent* e)
{
    // ── Sketch mode ───────────────────────────────────────────────────────────
    if (m_sketch && m_activeTool) {
        if (e->button() != Qt::MiddleButton && e->button() != Qt::RightButton) {
            QVector2D rawPos = screenToSketch(e->pos());
            SnapResult snap = m_snapEngine.snap(rawPos, m_sketch);
            m_activeTool->onMousePress(snap.pos, e->buttons(), e->modifiers());
            if (m_activeTool->isDone()) {
                m_activeTool->reset();
            }
            update();
            return;
        }
        // RMB in sketch mode with tool: cancel tool
        if (e->button() == Qt::RightButton && m_activeTool) {
            QVector2D rawPos = screenToSketch(e->pos());
            m_activeTool->onMousePress(rawPos, e->buttons(), e->modifiers());
            update();
            return;
        }
    }

    // ── Navigation (always available) ────────────────────────────────────────
    if (e->button() == Qt::RightButton && !m_sketch) {
        m_dragMode = DragMode::Orbit;
        m_camera.orbitBegin(e->pos());
    } else if (e->button() == Qt::RightButton && m_sketch) {
        // In sketch mode, RMB orbits if no tool active
        if (!m_activeTool) {
            m_dragMode = DragMode::Orbit;
            m_camera.orbitBegin(e->pos());
        }
    } else if (e->button() == Qt::MiddleButton) {
        m_dragMode = DragMode::Pan;
        m_camera.panBegin(e->pos());
    } else if (e->button() == Qt::LeftButton && !m_sketch) {
        m_lmbDown     = true;
        m_lmbPressPos = e->pos();
        m_dragMode    = DragMode::None;

        // Try gizmo pick first before body pick
        if (m_renderer.gizmo().visible()) {
            QVector3D ro, rd;
            m_camera.unprojectRay(e->pos().x(), e->pos().y(), width(), height(), ro, rd);
            GizmoHandle handle = m_renderer.gizmo().pick(ro, rd, m_camera.position(),
                                                          height(), m_camera.fov());
            if (handle != GizmoHandle::None) {
                m_dragMode = DragMode::GizmoDrag;
                // Snapshot the selected body for undo on drag end
                m_gizmoDragBody = nullptr;
                if (m_document) {
                    for (auto& b : m_document->bodies()) {
                        if (b->selected()) { m_gizmoDragBody = b.get(); break; }
                    }
                }
#ifdef ELCAD_HAVE_OCCT
                if (m_gizmoDragBody && m_gizmoDragBody->hasShape())
                    m_gizmoDragStartShape = m_gizmoDragBody->shape();
#endif
                m_renderer.gizmo().beginDrag(handle, ro, rd,
                                              m_camera.position(), height(), m_camera.fov());
                update();
                return;
            }
        }
    }
    update();
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton && m_dragMode == DragMode::Orbit) {
        m_dragMode = DragMode::None;
    } else if (e->button() == Qt::MiddleButton && m_dragMode == DragMode::Pan) {
        m_dragMode = DragMode::None;
    } else if (e->button() == Qt::LeftButton && m_lmbDown) {
        m_lmbDown = false;

        if (m_dragMode == DragMode::GizmoDrag) {
            commitGizmoDrag();
            m_dragMode = DragMode::None;
        } else if (m_dragMode == DragMode::BoxSelect) {
            m_rubberBand->hide();
            m_dragMode = DragMode::None;
            QRect rect = QRect(m_lmbPressPos, e->pos()).normalized();
            bool addToSelection = (e->modifiers() & Qt::ControlModifier) != 0;
            handleBoxSelect(rect, addToSelection);
        } else {
            QPoint delta = e->pos() - m_lmbPressPos;
            if (delta.manhattanLength() < kBoxSelectThreshold) {
                bool addToSelection = (e->modifiers() & Qt::ControlModifier) != 0;
                handlePickClick(e->pos(), addToSelection);
            }
        }
    }
    update();
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* e)
{
    // ── Sketch tool mouse move ────────────────────────────────────────────────
    if (m_sketch) {
        QVector2D rawPos = screenToSketch(e->pos());
        SnapResult snap  = m_snapEngine.snap(rawPos, m_sketch);
        m_snapPos    = snap.pos;
        m_hasSnapPos = true;

        emit sketchCursorPos(snap.pos.x(), snap.pos.y());

        if (m_activeTool) {
            m_activeTool->onMouseMove(snap.pos);
        }
        update();
    }

    // ── Camera navigation ─────────────────────────────────────────────────────
    if (m_dragMode == DragMode::Orbit) {
        m_camera.orbitMove(e->pos());
        emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
    } else if (m_dragMode == DragMode::Pan)
        m_camera.panMove(e->pos());
    else if (m_dragMode == DragMode::GizmoDrag && m_gizmoDragBody) {
        QVector3D ro, rd;
        m_camera.unprojectRay(e->pos().x(), e->pos().y(), width(), height(), ro, rd);
        Gizmo::DragDelta delta = m_renderer.gizmo().updateDrag(ro, rd,
                                    m_camera.position(), height(), m_camera.fov());
#ifdef ELCAD_HAVE_OCCT
        if (m_gizmoDragBody->hasShape()) {
            Gizmo& g = m_renderer.gizmo();
            TopoDS_Shape newShape = m_gizmoDragStartShape;
            if (g.mode() == GizmoMode::Translate) {
                newShape = TransformOps::translate(newShape,
                    delta.translate.x(), delta.translate.y(), delta.translate.z());
            } else if (g.mode() == GizmoMode::Rotate) {
                if (qAbs(delta.rotAngle) > 1e-5f)
                    newShape = TransformOps::rotate(newShape,
                        delta.rotAxis.x(), delta.rotAxis.y(), delta.rotAxis.z(),
                        g.position().x(),  g.position().y(),  g.position().z(),
                        delta.rotAngle);
            } else { // Scale
                if (qAbs(delta.scaleFactor - 1.0f) > 1e-5f)
                    newShape = TransformOps::scale(newShape,
                        g.position().x(), g.position().y(), g.position().z(),
                        delta.scaleFactor);
            }
            m_gizmoDragBody->setShape(newShape);
            m_renderer.invalidateMesh(m_gizmoDragBody->id());
        }
#endif
        update();
        return;
    } else if (m_lmbDown && !m_sketch) {
        QPoint delta = e->pos() - m_lmbPressPos;
        if (m_dragMode != DragMode::BoxSelect &&
            delta.manhattanLength() >= kBoxSelectThreshold)
        {
            m_dragMode = DragMode::BoxSelect;
        }
        if (m_dragMode == DragMode::BoxSelect) {
            m_rubberBand->setGeometry(QRect(m_lmbPressPos, e->pos()).normalized());
            m_rubberBand->show();
        }
    }

    if (m_dragMode == DragMode::Orbit || m_dragMode == DragMode::Pan)
        update();

    // ── Gizmo hover highlight ─────────────────────────────────────────────────
    if (!m_sketch && m_dragMode == DragMode::None) {
        QVector3D ro, rd;
        m_camera.unprojectRay(e->pos().x(), e->pos().y(), width(), height(), ro, rd);
        if (m_renderer.gizmo().updateHover(ro, rd, m_camera.position(),
                                            height(), m_camera.fov()))
            update();
    }

    // ── World cursor coordinates for status bar ───────────────────────────────
    if (!m_sketch) {
        QVector3D ro, rd;
        m_camera.unprojectRay(e->pos().x(), e->pos().y(), width(), height(), ro, rd);
        if (qAbs(rd.y()) > 1e-6f) {
            float t = -ro.y() / rd.y();
            if (t > 0.f) {
                QVector3D hit = ro + rd * t;
                emit cursorWorldPos(hit.x(), hit.y(), hit.z());
            }
        }
    }
}

void ViewportWidget::wheelEvent(QWheelEvent* e)
{
    m_camera.zoom(e->angleDelta().y());
    update();
}

// ── Gizmo drag commit ─────────────────────────────────────────────────────────

void ViewportWidget::commitGizmoDrag()
{
    m_renderer.gizmo().endDrag();

#ifdef ELCAD_HAVE_OCCT
    if (!m_gizmoDragBody || !m_document) { m_gizmoDragBody = nullptr; return; }

    quint64 bodyId      = m_gizmoDragBody->id();
    TopoDS_Shape startS = m_gizmoDragStartShape;
    TopoDS_Shape endS   = m_gizmoDragBody->hasShape() ? m_gizmoDragBody->shape()
                                                       : m_gizmoDragStartShape;

    bool firstRedo = true;
    m_document->undoStack().push(std::make_unique<LambdaCommand>(
        "Gizmo Transform",
        [this, bodyId, startS]() {
            if (Body* b = m_document->bodyById(bodyId)) {
                b->setShape(startS);
                m_renderer.invalidateMesh(bodyId);
                emit m_document->bodyChanged(b);
            }
        },
        [this, bodyId, endS, firstRedo]() mutable {
            if (firstRedo) { firstRedo = false; return; }  // shape already applied
            if (Body* b = m_document->bodyById(bodyId)) {
                b->setShape(endS);
                m_renderer.invalidateMesh(bodyId);
                emit m_document->bodyChanged(b);
            }
        }
    ));
#endif

    m_gizmoDragBody = nullptr;
}

// ── Selection helpers ─────────────────────────────────────────────────────────

void ViewportWidget::handlePickClick(QPoint pos, bool addToSelection)
{
    if (!m_document) return;

    QVector3D ro, rd;
    m_camera.unprojectRay(pos.x(), pos.y(), width(), height(), ro, rd);

    Body* hit = m_renderer.pickBody(ro, rd, m_document);

    if (!addToSelection)
        m_document->clearSelection();

    if (hit) {
        if (addToSelection && hit->selected())
            hit->setSelected(false);
        else
            hit->setSelected(true);
    }

    emit m_document->selectionChanged();
    update();
}

void ViewportWidget::handleBoxSelect(QRect rect, bool addToSelection)
{
    if (!m_document || rect.isEmpty()) return;

    QMatrix4x4 vp = m_camera.projectionMatrix() * m_camera.viewMatrix();
    int W = width(), H = height();

    if (!addToSelection)
        m_document->clearSelection();

    for (auto& bodyPtr : m_document->bodies()) {
        Body* body = bodyPtr.get();
        if (!body->visible() || !body->hasBbox()) continue;

        QVector3D bmin = body->bboxMin(), bmax = body->bboxMax();
        bool inside = false;
        for (int i = 0; i < 8 && !inside; ++i) {
            QVector3D corner(
                (i & 1) ? bmax.x() : bmin.x(),
                (i & 2) ? bmax.y() : bmin.y(),
                (i & 4) ? bmax.z() : bmin.z());
            QVector4D clip = vp * QVector4D(corner, 1.f);
            if (clip.w() <= 0.f) continue;
            float ndcX = clip.x() / clip.w();
            float ndcY = clip.y() / clip.w();
            int sx = static_cast<int>((ndcX * 0.5f + 0.5f) * W);
            int sy = static_cast<int>((1.f - (ndcY * 0.5f + 0.5f)) * H);
            if (rect.contains(sx, sy)) inside = true;
        }
        if (inside) body->setSelected(true);
    }

    emit m_document->selectionChanged();
    update();
}

// ── Keyboard ──────────────────────────────────────────────────────────────────

void ViewportWidget::keyPressEvent(QKeyEvent* e)
{
    // Forward to active sketch tool first
    if (m_activeTool && m_activeTool->onKeyPress(e)) {
        update();
        return;
    }

    switch (e->key()) {
    case Qt::Key_T:
        m_renderer.gizmo().setMode(GizmoMode::Translate);
        LOG_DEBUG("Viewport: gizmo mode → Translate");
        update();
        break;
    case Qt::Key_R:
        m_renderer.gizmo().setMode(GizmoMode::Rotate);
        LOG_DEBUG("Viewport: gizmo mode → Rotate");
        update();
        break;
    case Qt::Key_S:
        m_renderer.gizmo().setMode(GizmoMode::Scale);
        LOG_DEBUG("Viewport: gizmo mode → Scale");
        update();
        break;
    case Qt::Key_G:
        m_renderer.setGridVisible(!m_renderer.gridVisible());
        LOG_DEBUG("Viewport: grid toggled {}", m_renderer.gridVisible() ? "ON" : "OFF");
        update();
        break;
    case Qt::Key_F:
        m_camera.fitAll();
        emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
        update();
        break;
    case Qt::Key_P: {
        bool persp = !m_camera.isPerspective();
        m_camera.setPerspective(persp);
        LOG_DEBUG("Viewport: projection switched to {}", persp ? "Perspective" : "Orthographic");
        emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
        update();
        break;
    }
    case Qt::Key_Escape:
        if (m_sketch) {
            // In sketch mode: Esc with no in-progress tool action → exit sketch
            emit requestExitSketch();
        } else if (m_document) {
            m_document->clearSelection();
            emit m_document->selectionChanged();
            update();
        }
        break;
    case Qt::Key_1:
        if (e->modifiers() == Qt::KeypadModifier || e->modifiers() == Qt::NoModifier) {
            m_camera.setViewFront();
            emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
            update();
        }
        break;
    case Qt::Key_3:
        if (e->modifiers() == Qt::KeypadModifier || e->modifiers() == Qt::NoModifier) {
            m_camera.setViewRight();
            emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
            update();
        }
        break;
    case Qt::Key_7:
        if (e->modifiers() == Qt::KeypadModifier || e->modifiers() == Qt::NoModifier) {
            m_camera.setViewTop();
            emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
            update();
        }
        break;
    case Qt::Key_0:
        if (e->modifiers() == Qt::KeypadModifier || e->modifiers() == Qt::NoModifier) {
            m_camera.setViewIsometric();
            emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
            update();
        }
        break;
    default:
        QOpenGLWidget::keyPressEvent(e);
    }
}

void ViewportWidget::applyOrbit(float dyaw, float dpitch)
{
    m_camera.orbitBy(dyaw, dpitch);
    emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
    update();
}

void ViewportWidget::applyViewPreset(float yaw, float pitch)
{
    m_camera.setYawPitch(yaw, pitch);
    emit cameraOrientationChanged(m_camera.yaw(), m_camera.pitch(), m_camera.isPerspective());
    update();
}

} // namespace elcad
