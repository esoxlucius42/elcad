#include "viewport/Renderer.h"
#include "core/Logger.h"
#include "document/Document.h"
#include "document/Body.h"
#include "sketch/Sketch.h"
#include <QMatrix3x3>

namespace elcad {

void Renderer::initialize()
{
    if (m_initialized) return;
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);

    m_grid.initialize();
    m_phong.load(":/shaders/phong.vert", ":/shaders/phong.frag");
    m_edge.load(":/shaders/edge.vert",   ":/shaders/edge.frag");
    m_sketchRenderer.initialize();
    m_gizmo.initialize();

    m_initialized = true;
    LOG_INFO("Renderer initialised — phong shader valid={}, edge shader valid={}",
             m_phong.isValid(), m_edge.isValid());
}

void Renderer::resize(int w, int h)
{
    m_width  = w;
    m_height = h;
    glViewport(0, 0, w, h);
}

void Renderer::render(Camera& camera, Document* doc,
                      const std::vector<SketchEntity>* sketchPreview,
                      const QVector2D* snapPos)
{
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 view = camera.viewMatrix();
    QMatrix4x4 proj = camera.projectionMatrix();

    if (m_gridVisible)
        m_grid.render(view, proj, camera.nearPlane(), camera.farPlane());

    if (!doc) return;

    QVector3D camPos = camera.position();

    // Opaque pass: draw all body faces first, then edges on top
    glPolygonOffset(1.f, 1.f);
    glEnable(GL_POLYGON_OFFSET_FILL);

    for (auto& bodyPtr : doc->bodies()) {
        Body* body = bodyPtr.get();
        if (!body->visible()) continue;
        drawBody(body, view, proj, camPos);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);

    // ── Gizmo overlay (translate/rotate/scale handles) ────────────────────────
    if (!m_activeSketch) {
        Body* selected = nullptr;
        for (auto& b : doc->bodies()) {
            if (b->selected() && b->hasBbox()) { selected = b.get(); break; }
        }
        m_gizmo.setVisible(selected != nullptr);
        if (selected && !m_gizmo.isDragging()) {
            m_gizmo.setPosition((selected->bboxMin() + selected->bboxMax()) * 0.5f);
        }
        if (m_gizmo.visible())
            m_gizmo.draw(view, proj, camPos, m_height, camera.fov());
    } else {
        m_gizmo.setVisible(false);
    }

    // ── Sketch overlay ────────────────────────────────────────────────────────
    if (m_activeSketch) {
        static const std::vector<SketchEntity> empty;
        const std::vector<SketchEntity>& preview = sketchPreview ? *sketchPreview : empty;
        m_sketchRenderer.render(*m_activeSketch, view, proj, preview, snapPos);
    }
}

void Renderer::drawBody(Body* body, const QMatrix4x4& view, const QMatrix4x4& proj,
                        const QVector3D& camPos)
{
#ifndef ELCAD_HAVE_OCCT
    Q_UNUSED(body) Q_UNUSED(view) Q_UNUSED(proj) Q_UNUSED(camPos)
    return;
#else
    if (!body->hasShape()) return;

    MeshBuffer* mesh = getMeshBuffer(body);
    if (!mesh || mesh->isEmpty()) return;

    QMatrix4x4 model;  // identity — shapes are stored in world space

    // Normal matrix = transpose(inverse(model)) — identity here
    QMatrix3x3 normalMat = model.normalMatrix();

    // Body colour: brighter when selected
    QColor c = body->color();
    if (body->selected())
        c = c.lighter(140);

    QVector3D objColor(c.redF(), c.greenF(), c.blueF());

    // ── Phong-shaded triangles ────────────────────────────────────────────────
    if (m_phong.isValid()) {
        m_phong.bind();
        m_phong.setMat4("uModel",       model);
        m_phong.setMat4("uView",        view);
        m_phong.setMat4("uProjection",  proj);
        m_phong.setMat3("uNormalMatrix",normalMat);
        m_phong.setVec3("uLightDir",    m_lightDir.normalized());
        m_phong.setVec3("uLightColor",  m_lightColor);
        m_phong.setVec3("uObjectColor", objColor);
        m_phong.setVec3("uViewPos",     camPos);
        m_phong.setVec3("uSkyColor",    m_skyColor);
        m_phong.setVec3("uGroundColor", m_groundColor);
        m_phong.setVec3("uFillDir",     m_fillDir.normalized());
        m_phong.setVec3("uFillColor",   m_fillColor);

        mesh->drawTriangles();
        m_phong.release();
    }

    // ── Edge lines ────────────────────────────────────────────────────────────
    if (m_edge.isValid()) {
        QVector3D edgeColor = body->selected()
            ? QVector3D(1.0f, 0.7f, 0.0f)   // gold when selected
            : QVector3D(0.05f, 0.05f, 0.05f); // near-black otherwise

        m_edge.bind();
        m_edge.setMat4("uModel",      model);
        m_edge.setMat4("uView",       view);
        m_edge.setMat4("uProjection", proj);
        m_edge.setVec3("uColor",      edgeColor);

        glLineWidth(1.2f);
        mesh->drawEdges();
        m_edge.release();
    }
#endif
}

MeshBuffer* Renderer::getMeshBuffer(Body* body)
{
#ifndef ELCAD_HAVE_OCCT
    Q_UNUSED(body)
    return nullptr;
#else
    quint64 id = body->id();
    auto it = m_meshCache.find(id);

    if (it == m_meshCache.end() || body->meshDirty()) {
        LOG_DEBUG("Tessellating body id={} name='{}' — building GPU mesh",
                  id, body->name().toStdString());
        auto buf = std::make_unique<MeshBuffer>();
        buf->build(body->shape());
        body->setMeshDirty(false);
        body->setBbox(buf->bboxMin(), buf->bboxMax());
        body->setTriangleCount(buf->triangleCount() / 3);

        int triCount = buf->triangleCount() / 3;
        LOG_DEBUG("Tessellation done — body id={} name='{}' triangles={} "
                  "bbox=({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f})",
                  id, body->name().toStdString(), triCount,
                  buf->bboxMin().x(), buf->bboxMin().y(), buf->bboxMin().z(),
                  buf->bboxMax().x(), buf->bboxMax().y(), buf->bboxMax().z());

        if (triCount == 0)
            LOG_WARN("Body id={} name='{}' produced zero triangles — shape may be empty or degenerate",
                     id, body->name().toStdString());

        MeshBuffer* ptr = buf.get();
        m_meshCache[id] = std::move(buf);
        return ptr;
    }
    return it->second.get();
#endif
}

void Renderer::invalidateMesh(quint64 bodyId)
{
    LOG_DEBUG("Invalidating GPU mesh cache for body id={}", bodyId);
    m_meshCache.erase(bodyId);
}

void Renderer::clearMeshCache()
{
    LOG_DEBUG("Clearing entire GPU mesh cache ({} entries)", m_meshCache.size());
    m_meshCache.clear();
}

Body* Renderer::pickBody(const QVector3D& rayOrigin, const QVector3D& rayDir, Document* doc)
{
#ifndef ELCAD_HAVE_OCCT
    Q_UNUSED(rayOrigin) Q_UNUSED(rayDir) Q_UNUSED(doc)
    return nullptr;
#else
    if (!doc) return nullptr;

    Body* closest = nullptr;
    float minT    = std::numeric_limits<float>::max();

    for (auto& bodyPtr : doc->bodies()) {
        Body* body = bodyPtr.get();
        if (!body->visible()) continue;

        MeshBuffer* mesh = getMeshBuffer(body);
        if (!mesh || mesh->isEmpty()) continue;

        float t;
        if (mesh->rayIntersect(rayOrigin, rayDir, t) && t < minT) {
            minT    = t;
            closest = body;
        }
    }

    if (closest)
        LOG_DEBUG("Ray pick: hit body id={} name='{}' t={:.4f}",
                  closest->id(), closest->name().toStdString(), minT);
    else
        LOG_TRACE("Ray pick: no hit");

    return closest;
#endif
}

} // namespace elcad

