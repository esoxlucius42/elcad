#include "sketch/SketchRenderer.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"
#include <QtMath>
#include <QMatrix4x4>

namespace elcad {

static constexpr int kCircleSegments = 64;
static constexpr float kCrosshairSize = 5.f;  // mm

SketchRenderer::~SketchRenderer()
{
    if (!m_initialized) return;
    initializeOpenGLFunctions();
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void SketchRenderer::initialize()
{
    if (m_initialized) return;
    initializeOpenGLFunctions();

    m_shader.load(":/shaders/edge.vert", ":/shaders/edge.frag");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    m_initialized = true;
}

void SketchRenderer::render(const Sketch& sketch,
                             const QMatrix4x4& view,
                             const QMatrix4x4& proj,
                             const std::vector<SketchEntity>& previewEntities,
                             const QVector2D* snapPos)
{
    if (!m_initialized) return;

    LineList normal, selected, construction, preview;

    const SketchPlane& plane = sketch.plane();

    // ── Draw axis lines at origin ─────────────────────────────────────────────
    auto addAxisLine = [&](QVector2D a, QVector2D b) {
        construction.push_back(plane.to3D(a));
        construction.push_back(plane.to3D(b));
    };
    addAxisLine({-1000.f, 0.f}, {1000.f, 0.f});  // U axis
    addAxisLine({0.f, -1000.f}, {0.f, 1000.f});  // V axis

    // ── Committed entities ────────────────────────────────────────────────────
    for (const auto& ep : sketch.entities())
        addEntityLines(*ep, plane, normal, selected, construction, preview, false);

    // ── Preview (in-progress tool) ────────────────────────────────────────────
    for (const auto& e : previewEntities)
        addEntityLines(e, plane, normal, selected, construction, preview, true);

    // ── Snap crosshair ────────────────────────────────────────────────────────
    if (snapPos) {
        float s = kCrosshairSize;
        QVector2D p = *snapPos;
        preview.push_back(plane.to3D(p + QVector2D(-s, 0)));
        preview.push_back(plane.to3D(p + QVector2D( s, 0)));
        preview.push_back(plane.to3D(p + QVector2D(0, -s)));
        preview.push_back(plane.to3D(p + QVector2D(0,  s)));
    }

    // ── Draw all groups ───────────────────────────────────────────────────────
    glDisable(GL_DEPTH_TEST);  // always on top
    glLineWidth(1.5f);

    drawLines(construction, {0.35f, 0.45f, 0.55f}, view, proj);  // dim blue-grey
    drawLines(normal,       {0.90f, 0.90f, 0.90f}, view, proj);  // near-white
    drawLines(selected,     {1.00f, 0.70f, 0.00f}, view, proj);  // gold
    drawLines(preview,      {0.30f, 0.80f, 1.00f}, view, proj);  // cyan

    glEnable(GL_DEPTH_TEST);
}

void SketchRenderer::addEntityLines(const SketchEntity& e, const SketchPlane& plane,
                                     LineList& normal, LineList& selected,
                                     LineList& construction, LineList& preview,
                                     bool isPreview) const
{
    LineList& out = isPreview ? preview
                  : e.selected ? selected
                  : e.construction ? construction
                  : normal;

    if (e.type == SketchEntity::Line) {
        out.push_back(plane.to3D(e.p0.x, e.p0.y));
        out.push_back(plane.to3D(e.p1.x, e.p1.y));
    } else if (e.type == SketchEntity::Circle) {
        addCircleLines(e.p0.toVec(), e.radius, plane, out);
    } else if (e.type == SketchEntity::Arc) {
        addCircleLines(e.p0.toVec(), e.radius, plane, out, e.startAngle, e.endAngle);
    }
}

void SketchRenderer::addCircleLines(QVector2D center, float radius,
                                     const SketchPlane& plane, LineList& out,
                                     float startDeg, float endDeg) const
{
    if (radius <= 0.f) return;

    bool full = (qAbs(endDeg - startDeg - 360.f) < 1e-3f ||
                 qAbs(endDeg - startDeg)          < 1e-3f);
    int N = kCircleSegments;
    float aStart = qDegreesToRadians(startDeg);
    float aEnd   = qDegreesToRadians(full ? startDeg + 360.f : endDeg);
    float step   = (aEnd - aStart) / float(N);

    for (int i = 0; i < N; ++i) {
        float a0 = aStart + i * step;
        float a1 = aStart + (i + 1) * step;
        QVector2D pt0 = center + QVector2D(radius * qCos(a0), radius * qSin(a0));
        QVector2D pt1 = center + QVector2D(radius * qCos(a1), radius * qSin(a1));
        out.push_back(plane.to3D(pt0));
        out.push_back(plane.to3D(pt1));
    }
}

void SketchRenderer::drawLines(const LineList& pts, QVector3D color,
                                const QMatrix4x4& view, const QMatrix4x4& proj)
{
    if (pts.empty() || !m_shader.isValid()) return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(pts.size() * sizeof(QVector3D)),
                 pts.data(), GL_STREAM_DRAW);

    QMatrix4x4 model; // identity
    m_shader.bind();
    m_shader.setMat4("uModel",      model);
    m_shader.setMat4("uView",       view);
    m_shader.setMat4("uProjection", proj);
    m_shader.setVec3("uColor",      color);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(pts.size()));

    m_shader.release();
    glBindVertexArray(0);
}

} // namespace elcad
