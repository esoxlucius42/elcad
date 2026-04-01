#include "sketch/SketchRenderer.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"
#include "sketch/SketchPicker.h"
#include <QtMath>
#include <QMatrix4x4>
#include <algorithm>

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

    // ── Origin point marker (yellow cross + diamond) ──────────────────────────
    LineList originMarker;
    constexpr float kCross    = 4.f;  // mm — cross arm half-length
    constexpr float kDiamond  = 3.f;  // mm — diamond half-extent
    originMarker.push_back(plane.to3D({-kCross,     0.f    }));
    originMarker.push_back(plane.to3D({ kCross,     0.f    }));
    originMarker.push_back(plane.to3D({  0.f,   -kCross    }));
    originMarker.push_back(plane.to3D({  0.f,    kCross    }));
    // diamond
    originMarker.push_back(plane.to3D({ kDiamond,  0.f    }));
    originMarker.push_back(plane.to3D({  0.f,    kDiamond  }));
    originMarker.push_back(plane.to3D({  0.f,    kDiamond  }));
    originMarker.push_back(plane.to3D({-kDiamond,  0.f    }));
    originMarker.push_back(plane.to3D({-kDiamond,  0.f    }));
    originMarker.push_back(plane.to3D({  0.f,   -kDiamond  }));
    originMarker.push_back(plane.to3D({  0.f,   -kDiamond  }));
    originMarker.push_back(plane.to3D({ kDiamond,  0.f    }));

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

    glLineWidth(2.0f);
    drawLines(originMarker, {1.00f, 1.00f, 0.00f}, view, proj);  // bright yellow

    glLineWidth(1.5f);
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

void SketchRenderer::drawTriangles(const std::vector<QVector3D>& pts, QVector3D color, float alpha,
                                    const QMatrix4x4& view, const QMatrix4x4& proj)
{
    if (pts.empty() || !m_shader.isValid()) return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(pts.size() * sizeof(QVector3D)),
                 pts.data(), GL_STREAM_DRAW);

    QMatrix4x4 model;
    m_shader.bind();
    m_shader.setMat4("uModel",      model);
    m_shader.setMat4("uView",       view);
    m_shader.setMat4("uProjection", proj);
    m_shader.setVec3("uColor",      color);

    // Temporarily set blend color via uniform alpha if the shader supports it;
    // otherwise tint the color to simulate transparency by blending with the bg.
    // Since the edge shader only has uColor (no alpha uniform), we multiply the
    // color by alpha to approximate the tint over the dark background (#333333).
    QVector3D tinted = color * alpha + QVector3D(0.2f, 0.2f, 0.2f) * (1.f - alpha);
    m_shader.setVec3("uColor", tinted);

    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(pts.size()));
    glEnable(GL_CULL_FACE);

    m_shader.release();
    glBindVertexArray(0);
}

// ── renderInactive ────────────────────────────────────────────────────────────

void SketchRenderer::renderInactive(const Sketch& sketch,
                                     const QMatrix4x4& view,
                                     const QMatrix4x4& proj,
                                     quint64 hoveredEntityId,
                                     const std::vector<quint64>& selectedEntityIds,
                                     int hoveredAreaIndex,
                                     quint64 hoveredCircleEntityId,
                                     const std::vector<int>& selectedAreaIndices,
                                     const std::vector<quint64>& selectedCircleEntityIds)
{
    if (!m_initialized) return;

    const SketchPlane& plane = sketch.plane();

    auto isEntitySelected = [&](quint64 id) {
        return std::find(selectedEntityIds.begin(), selectedEntityIds.end(), id)
               != selectedEntityIds.end();
    };
    auto isAreaSelected = [&](int idx) {
        return std::find(selectedAreaIndices.begin(), selectedAreaIndices.end(), idx)
               != selectedAreaIndices.end();
    };
    auto isCircleAreaSelected = [&](quint64 id) {
        return std::find(selectedCircleEntityIds.begin(), selectedCircleEntityIds.end(), id)
               != selectedCircleEntityIds.end();
    };

    // ── Collect entity lines ──────────────────────────────────────────────────
    LineList normal, hovered, selected;

    for (const auto& ep : sketch.entities()) {
        const SketchEntity& e = *ep;
        if (e.construction) continue;

        bool sel = isEntitySelected(e.id);
        bool hov = (e.id == hoveredEntityId);

        LineList& out = hov ? hovered : sel ? selected : normal;

        if (e.type == SketchEntity::Line) {
            out.push_back(plane.to3D(e.p0.x, e.p0.y));
            out.push_back(plane.to3D(e.p1.x, e.p1.y));
        } else if (e.type == SketchEntity::Circle) {
            addCircleLines(e.p0.toVec(), e.radius, plane, out);
        } else if (e.type == SketchEntity::Arc) {
            addCircleLines(e.p0.toVec(), e.radius, plane, out, e.startAngle, e.endAngle);
        }
    }

    // ── Collect endpoint dots (small cross markers at line endpoints) ─────────
    LineList dots;
    constexpr float kDotSize = 1.5f;  // mm, half-arm length of the cross
    for (const auto& ep : sketch.entities()) {
        const SketchEntity& e = *ep;
        if (e.construction) continue;
        if (e.type != SketchEntity::Line) continue;

        auto addDot = [&](QVector2D p) {
            dots.push_back(plane.to3D(p + QVector2D(-kDotSize, 0)));
            dots.push_back(plane.to3D(p + QVector2D( kDotSize, 0)));
            dots.push_back(plane.to3D(p + QVector2D(0, -kDotSize)));
            dots.push_back(plane.to3D(p + QVector2D(0,  kDotSize)));
        };
        addDot(e.p0.toVec());
        addDot(e.p1.toVec());
    }

    // ── Collect area fills ────────────────────────────────────────────────────
    // Fan-triangulate closed polygon loops from centroid.
    std::vector<QVector3D> hoveredFill, selectedFill;

    // Closed line loops
    auto loops = SketchPicker::findClosedLoops(sketch);
    for (int i = 0; i < static_cast<int>(loops.size()); ++i) {
        const auto& poly = loops[i].polygon;
        if (poly.size() < 3) continue;

        bool isHov = (hoveredAreaIndex == i);
        bool isSel = isAreaSelected(i);
        if (!isHov && !isSel) continue;

        // Centroid
        QVector2D centroid{};
        for (const auto& v : poly) centroid += v;
        centroid /= static_cast<float>(poly.size());

        auto& fillOut = isSel ? selectedFill : hoveredFill;
        for (int j = 0; j < static_cast<int>(poly.size()); ++j) {
            QVector2D a = poly[j];
            QVector2D b = poly[(j + 1) % static_cast<int>(poly.size())];
            fillOut.push_back(plane.to3D(centroid));
            fillOut.push_back(plane.to3D(a));
            fillOut.push_back(plane.to3D(b));
        }
    }

    // Circle areas
    constexpr int kCircleFillSegs = 32;
    for (const auto& ep : sketch.entities()) {
        const SketchEntity& e = *ep;
        if (e.construction || e.type != SketchEntity::Circle) continue;

        bool isHov = (e.id == hoveredCircleEntityId);
        bool isSel = isCircleAreaSelected(e.id);
        if (!isHov && !isSel) continue;

        auto& fillOut = isSel ? selectedFill : hoveredFill;
        QVector2D center = e.p0.toVec();
        float r = e.radius;
        for (int j = 0; j < kCircleFillSegs; ++j) {
            float a0 = float(j)     / kCircleFillSegs * 2.f * float(M_PI);
            float a1 = float(j + 1) / kCircleFillSegs * 2.f * float(M_PI);
            fillOut.push_back(plane.to3D(center));
            fillOut.push_back(plane.to3D(center + QVector2D(r * qCos(a0), r * qSin(a0))));
            fillOut.push_back(plane.to3D(center + QVector2D(r * qCos(a1), r * qSin(a1))));
        }
    }

    // ── Draw ─────────────────────────────────────────────────────────────────
    glDisable(GL_DEPTH_TEST);  // always on top of 3D bodies
    glLineWidth(1.2f);

    // Area fills (drawn first, behind lines)
    if (!hoveredFill.empty())
        drawTriangles(hoveredFill,  {0.27f, 0.60f, 0.85f}, 0.18f, view, proj);
    if (!selectedFill.empty())
        drawTriangles(selectedFill, {1.00f, 0.75f, 0.10f}, 0.18f, view, proj);

    // Entity lines: muted blue (#4488BB)
    drawLines(normal,   {0.27f, 0.53f, 0.73f}, view, proj);
    // Endpoint dots: slightly dimmer
    drawLines(dots,     {0.22f, 0.44f, 0.60f}, view, proj);
    // Hovered: bright teal
    glLineWidth(2.0f);
    drawLines(hovered,  {0.35f, 0.85f, 1.00f}, view, proj);
    // Selected: gold
    drawLines(selected, {1.00f, 0.70f, 0.00f}, view, proj);

    glLineWidth(1.f);
    glEnable(GL_DEPTH_TEST);
}

} // namespace elcad
