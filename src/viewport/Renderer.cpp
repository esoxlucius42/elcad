#include "viewport/Renderer.h"
#include "core/Logger.h"
#include "document/Document.h"
#include "document/Body.h"
#include "sketch/Sketch.h"
#include <QMatrix3x3>

#ifdef ELCAD_HAVE_OCCT
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#endif

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

    // Highlight buffers
    glGenVertexArrays(1, &m_highlightVao);
    glGenBuffers(1, &m_highlightVbo);

    // Face highlight buffers
    glGenVertexArrays(1, &m_faceHighlightVao);
    glGenBuffers(1, &m_faceHighlightVbo);

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
                      const QVector2D* snapPos,
                      float devicePixelRatio)
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // #333333
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
        drawBody(body, view, proj, camPos, doc);
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
            LOG_INFO("Renderer: selected id={} bbox_center=({:.3f},{:.3f},{:.3f}) viewH={} fov={}",
                     selected->id(),
                     ((selected->bboxMin()+selected->bboxMax())*0.5f).x(),
                     ((selected->bboxMin()+selected->bboxMax())*0.5f).y(),
                     ((selected->bboxMin()+selected->bboxMax())*0.5f).z(),
                     m_height, camera.fov());
        }
        if (m_gizmo.visible())
            m_gizmo.draw(view, proj, camPos, m_width, m_height, camera.fov());
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
                        const QVector3D& camPos, Document* doc)
{
#ifndef ELCAD_HAVE_OCCT
    Q_UNUSED(body) Q_UNUSED(view) Q_UNUSED(proj) Q_UNUSED(camPos) Q_UNUSED(doc)
    return;
#else
    if (!body->hasShape()) return;

    MeshBuffer* mesh = getMeshBuffer(body);
    if (!mesh || mesh->isEmpty()) return;

    QMatrix4x4 model;  // identity — shapes are stored in world space

    // Normal matrix = transpose(inverse(model)) — identity here
    QMatrix3x3 normalMat = model.normalMatrix();

    // Body colour: brighter when selected (whole-body selection)
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

    // ── Per-item highlights (faces/edges/vertices) ───────────────────────────
    const auto selItems = doc ? doc->selectionItems() : std::vector<Document::SelectedItem>{};
    if (!selItems.empty()) {
        std::vector<QVector3D> triCentroids;
        std::vector<QVector3D> vertPoints;
        std::vector<QVector3D> edgeLines; // pairs of points

        const auto& pv = mesh->pickVertices();
        const auto& pi = mesh->pickIndices();

        for (const auto& s : selItems) {
            if (s.bodyId != body->id()) continue;
            if (s.type == Document::SelectedItem::Type::Face) {
                int tri = s.index;
                size_t base = static_cast<size_t>(tri) * 3;
                if (base + 2 < pi.size()) {
                    const QVector3D& a = pv[pi[base]];
                    const QVector3D& b = pv[pi[base+1]];
                    const QVector3D& c = pv[pi[base+2]];
                    triCentroids.push_back((a + b + c) / 3.0f);
                }
            } else if (s.type == Document::SelectedItem::Type::Vertex) {
                int vid = s.index;
                if (vid >= 0 && static_cast<size_t>(vid) < pv.size())
                    vertPoints.push_back(pv[vid]);
            } else if (s.type == Document::SelectedItem::Type::Edge) {
                int enc = s.index;
                int tri = enc / 3;
                int local = enc % 3;
                size_t base = static_cast<size_t>(tri) * 3;
                if (base + 2 < pi.size()) {
                    int i0 = pi[base + local];
                    int i1 = pi[base + ((local + 1) % 3)];
                    edgeLines.push_back(pv[i0]);
                    edgeLines.push_back(pv[i1]);
                }
            }
        }

        // Draw filled face highlights (preferred) -- collect tri vertices and draw as translucent yellow
        if (!triCentroids.empty()) {
            std::vector<QVector3D> fillVerts;
            fillVerts.reserve(triCentroids.size() * 3);
            // Instead of using centroids we will draw full triangles stored in selItems
            const auto& pv = mesh->pickVertices();
            const auto& pi = mesh->pickIndices();
            for (const auto& s : selItems) {
                if (s.bodyId != body->id()) continue;
                if (s.type == Document::SelectedItem::Type::Face) {
                    int tri = s.index;
                    size_t base = static_cast<size_t>(tri) * 3;
                    if (base + 2 < pi.size()) {
                        fillVerts.push_back(pv[pi[base+0]]);
                        fillVerts.push_back(pv[pi[base+1]]);
                        fillVerts.push_back(pv[pi[base+2]]);
                    }
                }
            }

            if (!fillVerts.empty()) {
                // Use simple blend and polygon offset to avoid z-fighting
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(-1.f, -1.f);
                m_edge.bind();
                m_edge.setMat4("uModel", model);
                m_edge.setMat4("uView", view);
                m_edge.setMat4("uProjection", proj);
                m_edge.setVec3("uColor", QVector3D(1.0f, 0.7f, 0.0f));

                glBindVertexArray(m_faceHighlightVao);
                glBindBuffer(GL_ARRAY_BUFFER, m_faceHighlightVbo);
                glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(fillVerts.size() * sizeof(QVector3D)),
                             fillVerts.data(), GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
                glEnableVertexAttribArray(0);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(fillVerts.size()));
                glDisable(GL_BLEND);

                glDisableVertexAttribArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
                m_edge.release();
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
        }

        // Draw vertex points
        if (!vertPoints.empty()) {
            m_edge.bind();
            m_edge.setMat4("uModel", model);
            m_edge.setMat4("uView", view);
            m_edge.setMat4("uProjection", proj);
            m_edge.setVec3("uColor", QVector3D(0.95f, 0.95f, 0.2f));

            glBindVertexArray(m_highlightVao);
            glBindBuffer(GL_ARRAY_BUFFER, m_highlightVbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertPoints.size() * sizeof(QVector3D)),
                         vertPoints.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);
            glPointSize(10.0f);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertPoints.size()));
            glDisableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            m_edge.release();
        }

        // Draw edge lines
        if (!edgeLines.empty()) {
            m_edge.bind();
            m_edge.setMat4("uModel", model);
            m_edge.setMat4("uView", view);
            m_edge.setMat4("uProjection", proj);
            m_edge.setVec3("uColor", QVector3D(0.95f, 0.95f, 0.2f));

            glBindVertexArray(m_highlightVao);
            glBindBuffer(GL_ARRAY_BUFFER, m_highlightVbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(edgeLines.size() * sizeof(QVector3D)),
                         edgeLines.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);
            glLineWidth(2.5f);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(edgeLines.size()));
            glDisableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            m_edge.release();
        }
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

        // Adaptive tessellation: start with default deflection and increase (coarsen)
        // if triangle count is excessively large. This avoids OOM/slow selection on very
        // fine tessellations (e.g., spheres created with small deflection).
        const int kMaxTriangles = 30000; // safe upper bound for triangle count
        const int kMaxAttempts = 6;
        float deflection = 0.01f; // starting chord deflection (mm)
        int triCount = 0;
        std::unique_ptr<MeshBuffer> finalBuf;

        for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
            auto buf = std::make_unique<MeshBuffer>();
            buf->build(body->shape(), deflection);
            triCount = buf->triangleCount() / 3;

            LOG_DEBUG("Tessellated attempt {}: deflection={} -> triangles={}", attempt, deflection, triCount);

            // Acceptable triangle count
            if (triCount <= kMaxTriangles) {
                finalBuf = std::move(buf);
                break;
            }

            // Keep the last built mesh as a fallback if all attempts fail
            finalBuf = std::move(buf);
            LOG_INFO("Body id={} name='{}' produced {} triangles (> {}), increasing deflection",
                     id, body->name().toStdString(), triCount, kMaxTriangles);
            deflection *= 2.0f; // coarsen mesh
        }

        if (!finalBuf) {
            // Fallback: build once with larger deflection
            finalBuf = std::make_unique<MeshBuffer>();
            finalBuf->build(body->shape(), deflection);
            triCount = finalBuf->triangleCount() / 3;
        }

        body->setMeshDirty(false);
        body->setBbox(finalBuf->bboxMin(), finalBuf->bboxMax());
        body->setTriangleCount(finalBuf->triangleCount() / 3);

        LOG_DEBUG("Tessellation done — body id={} name='{}' triangles={} "
                  "bbox=({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f})",
                  id, body->name().toStdString(), triCount,
                  finalBuf->bboxMin().x(), finalBuf->bboxMin().y(), finalBuf->bboxMin().z(),
                  finalBuf->bboxMax().x(), finalBuf->bboxMax().y(), finalBuf->bboxMax().z());

        if (triCount == 0)
            LOG_WARN("Body id={} name='{}' produced zero triangles — shape may be empty or degenerate",
                     id, body->name().toStdString());

        MeshBuffer* ptr = finalBuf.get();
        m_meshCache[id] = std::move(finalBuf);
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

    // If a mesh is excessively large, skip per-triangle intersection and fall back
    // to a fast AABB test to avoid long stalls or crashes during selection.
    const int kSelectionTriangleLimit = 20000; // triangles

    auto rayAabb = [](const QVector3D& o, const QVector3D& d,
                      const QVector3D& bmin, const QVector3D& bmax, float& tMin) -> bool {
        float tmax = std::numeric_limits<float>::max();
        tMin = -std::numeric_limits<float>::max();
        for (int i = 0; i < 3; ++i) {
            float inv = (d[i] != 0.f) ? 1.f / d[i] : std::numeric_limits<float>::max();
            float t1 = (bmin[i] - o[i]) * inv;
            float t2 = (bmax[i] - o[i]) * inv;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tmax = std::min(tmax, t2);
            if (tmax < tMin) return false;
        }
        return tmax > 0.f;
    };

    for (auto& bodyPtr : doc->bodies()) {
        Body* body = bodyPtr.get();
        if (!body->visible()) continue;

        MeshBuffer* mesh = getMeshBuffer(body);
        if (!mesh || mesh->isEmpty()) continue;

        float t;
        bool hit = false;
        int triCount = mesh->triangleCount() / 3;
        if (triCount > kSelectionTriangleLimit && body->hasBbox()) {
            // Fast bbox test
            float tAabb;
            if (rayAabb(rayOrigin, rayDir, body->bboxMin(), body->bboxMax(), tAabb)) {
                t = tAabb;
                hit = true;
            }
        } else {
            if (mesh->rayIntersect(rayOrigin, rayDir, t)) hit = true;
        }

        if (hit && t < minT) {
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

bool Renderer::pickHit(const QVector3D& rayOrigin, const QVector3D& rayDir, Document* doc, Document::SelectedItem& outHit)
{
#ifndef ELCAD_HAVE_OCCT
    Q_UNUSED(rayOrigin) Q_UNUSED(rayDir) Q_UNUSED(doc) Q_UNUSED(outHit)
    return false;
#else
    outHit = {};
    if (!doc) return false;

    Body* closest = nullptr;
    float minT    = std::numeric_limits<float>::max();
    int   bestTri = -1;
    float bestU = 0.f, bestV = 0.f;

    const int kSelectionTriangleLimit = 20000; // triangles

    auto rayAabb = [](const QVector3D& o, const QVector3D& d,
                      const QVector3D& bmin, const QVector3D& bmax, float& tMin) -> bool {
        float tmax = std::numeric_limits<float>::max();
        tMin = -std::numeric_limits<float>::max();
        for (int i = 0; i < 3; ++i) {
            float inv = (d[i] != 0.f) ? 1.f / d[i] : std::numeric_limits<float>::max();
            float t1 = (bmin[i] - o[i]) * inv;
            float t2 = (bmax[i] - o[i]) * inv;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tmax = std::min(tmax, t2);
            if (tmax < tMin) return false;
        }
        return tmax > 0.f;
    };

    for (auto& bodyPtr : doc->bodies()) {
        Body* body = bodyPtr.get();
        if (!body->visible()) continue;

        MeshBuffer* mesh = getMeshBuffer(body);
        if (!mesh || mesh->isEmpty()) continue;

        float t; int triIdx; float u,v;
        int triCount = mesh->triangleCount() / 3;
        if (triCount > kSelectionTriangleLimit && body->hasBbox()) {
            // coarse bbox-based pick; return body-level hit only
            float tAabb;
            if (rayAabb(rayOrigin, rayDir, body->bboxMin(), body->bboxMax(), tAabb)) {
                LOG_DEBUG("Ray pick: AABB hit body id={} tAabb={:.4f} triCount={}", body->id(), tAabb, triCount);
                if (tAabb < minT) {
                    minT = tAabb;
                    closest = body;
                    bestTri = -1;
                }
            }
        } else {
            if (mesh->rayIntersectDetailed(rayOrigin, rayDir, t, triIdx, u, v) && t < minT) {
                // compute centroid and face ordinal for verbose diagnostics
                const auto& pv = mesh->pickVertices();
                const auto& pi = mesh->pickIndices();
                QVector3D centroid(0,0,0);
                size_t base = static_cast<size_t>(triIdx) * 3;
                if (base + 2 < pi.size()) {
                    centroid = (pv[pi[base+0]] + pv[pi[base+1]] + pv[pi[base+2]]) / 3.0f;
                }
                int faceOrd = mesh->faceOrdinalForTriangle(triIdx);
                LOG_DEBUG("Ray pick: tri hit body id={} tri={} t={:.4f} u={:.4f} v={:.4f} centroid=({:.3f},{:.3f},{:.3f}) faceOrd={}",
                          body->id(), triIdx, t, u, v, centroid.x(), centroid.y(), centroid.z(), faceOrd);

                minT = t;
                closest = body;
                bestTri = triIdx;
                bestU = u; bestV = v;
            }
        }
    }

    if (!closest) return false;

    Document::SelectedItem item;
    item.bodyId = closest->id();
    item.index = -1;
    item.type = Document::SelectedItem::Type::Body;

    if (bestTri >= 0) {
        float alpha = 1.0f - bestU - bestV;
        const float vertexEps = 1e-3f;
        const float edgeEps = 0.02f;

        if (alpha > 1.0f - vertexEps) { item.type = Document::SelectedItem::Type::Vertex; item.index = bestTri * 3 + 0; }
        else if (bestU > 1.0f - vertexEps) { item.type = Document::SelectedItem::Type::Vertex; item.index = bestTri * 3 + 1; }
        else if (bestV > 1.0f - vertexEps) { item.type = Document::SelectedItem::Type::Vertex; item.index = bestTri * 3 + 2; }
        else if (alpha < edgeEps || bestU < edgeEps || bestV < edgeEps) {
            item.type = Document::SelectedItem::Type::Edge;
            int edgeLocal = 0;
            if (alpha < edgeEps) edgeLocal = 0;
            else if (bestU < edgeEps) edgeLocal = 1;
            else edgeLocal = 2;
            item.index = bestTri * 3 + edgeLocal;
        } else {
            item.type = Document::SelectedItem::Type::Face;
            item.index = bestTri;
        }
    } else {
        // bestTri < 0 means we only detected a bbox-level hit for a large mesh; leave as Body
    }

    outHit = item;
#ifdef ELCAD_HAVE_OCCT
    if (closest && bestTri >= 0) {
        MeshBuffer* m = getMeshBuffer(closest);
        if (m) {
            const auto& pv = m->pickVertices();
            const auto& pi = m->pickIndices();
            size_t base = static_cast<size_t>(bestTri) * 3;
            if (base + 2 < pi.size()) {
                QVector3D c = (pv[pi[base+0]] + pv[pi[base+1]] + pv[pi[base+2]]) / 3.0f;
                int faceOrd = m->faceOrdinalForTriangle(bestTri);
                LOG_DEBUG("Ray pick: hit body id={} type={} idx={} t={:.4f} (tri={} centroid=({:.3f},{:.3f},{:.3f}) faceOrd={})",
                          outHit.bodyId, static_cast<int>(outHit.type), outHit.index, minT, bestTri, c.x(), c.y(), c.z(), faceOrd);
            } else {
                LOG_DEBUG("Ray pick: hit body id={} type={} idx={} t={:.4f} (tri={})",
                          outHit.bodyId, static_cast<int>(outHit.type), outHit.index, minT, bestTri);
            }
        } else {
            LOG_DEBUG("Ray pick: hit body id={} type={} idx={} t={:.4f} (tri={})",
                      outHit.bodyId, static_cast<int>(outHit.type), outHit.index, minT, bestTri);
        }
    } else {
        LOG_DEBUG("Ray pick: hit body id={} type={} idx={} t={:.4f}", outHit.bodyId, static_cast<int>(outHit.type), outHit.index, minT);
    }
#else
    LOG_DEBUG("Ray pick: hit body id={} type={} idx={} t={:.4f}", outHit.bodyId, static_cast<int>(outHit.type), outHit.index, minT);
#endif
    return true;
#endif
}

int Renderer::faceOrdinalForTriangle(Body* body, int triIndex)
{
#ifndef ELCAD_HAVE_OCCT
    Q_UNUSED(body); Q_UNUSED(triIndex);
    return -1;
#else
    if (!body) return -1;
    MeshBuffer* mesh = getMeshBuffer(body);
    if (!mesh) return -1;
    return mesh->faceOrdinalForTriangle(triIndex);
#endif
}


std::vector<int> Renderer::expandFaceSelection(Body* body, int startTri, float angleDeg, float distanceTol)
{
#ifndef ELCAD_HAVE_OCCT
    Q_UNUSED(body) Q_UNUSED(startTri) Q_UNUSED(angleDeg) Q_UNUSED(distanceTol)
    return {};
#else
    std::vector<int> result;
    if (!body) return result;

    MeshBuffer* mesh = getMeshBuffer(body);
    if (!mesh || mesh->isEmpty()) return result;

    mesh->ensureAdjacencyComputed();
    const auto& neighbors = mesh->triangleNeighbors();
    int triCount = static_cast<int>(mesh->triangleCount() / 3);
    if (startTri < 0 || startTri >= triCount) return result;

    const auto& pv = mesh->pickVertices();
    const auto& pi = mesh->pickIndices();
    float angleTolRad = qDegreesToRadians(angleDeg);
    float cosAngleTol = std::cos(angleTolRad);

    // BFS (per-edge/local threshold: compare neighbor normal to current triangle normal)
    std::vector<char> visited(triCount, 0);
    std::vector<int> stack;
    stack.push_back(startTri);
    visited[startTri] = 1;

    while (!stack.empty()) {
        int cur = stack.back(); stack.pop_back();
        result.push_back(cur);

        // current triangle reference normal/plane
        QVector3D curN = mesh->triangleNormal(cur);
        size_t curBase = static_cast<size_t>(cur) * 3;
        QVector3D curV0 = pv[pi[curBase+0]];

        for (int nb : neighbors[cur]) {
            if (nb < 0 || nb >= triCount) continue;
            if (visited[nb]) continue;

            QVector3D n = mesh->triangleNormal(nb);
            // angle via dot product (normals are unit) using current triangle normal
            float d = QVector3D::dotProduct(curN, n);
            if (d < cosAngleTol) continue;

            // For curved surfaces (cylinders/spheres) plane distance check is too strict and prevents
            // expansion. Rely on per-edge normal angle for traversal; keep the distance check optional
            // by only enforcing it if distanceTol > 0 and model is expected to be planar. For now,
            // skip the distance check to allow selection across smoothly curved surfaces.
            
            visited[nb] = 1;
            stack.push_back(nb);
        }
    }

    return result;
#endif
}

#ifdef ELCAD_HAVE_OCCT
// Build a TopoDS_Face from a set of mesh triangle indices. This constructs the boundary loop(s)
// from the triangles and builds a polygonal wire/face. Returns a null face on failure.
TopoDS_Face Renderer::buildFaceFromTriangles(Body* body, const std::vector<int>& triIndices)
{
    TopoDS_Face nullFace;
    if (!body || triIndices.empty()) return nullFace;

    MeshBuffer* mesh = getMeshBuffer(body);
    if (!mesh || mesh->isEmpty()) return nullFace;

    const auto& pv = mesh->pickVertices();
    const auto& pi = mesh->pickIndices();

    // Build edge map counting usages within the tri set
    struct EdgeKey { unsigned int a,b; };
    struct EdgeHash { size_t operator()(EdgeKey const& k) const noexcept { return (static_cast<size_t>(k.a) << 32) ^ static_cast<size_t>(k.b); } };
    struct EdgeEq { bool operator()(EdgeKey const& x, EdgeKey const& y) const noexcept { return x.a==y.a && x.b==y.b; } };

    std::unordered_map<EdgeKey, int, EdgeHash, EdgeEq> edgeCount;
    edgeCount.reserve(triIndices.size()*3);

    for (int tri : triIndices) {
        size_t base = static_cast<size_t>(tri) * 3;
        if (base + 2 >= pi.size()) continue;
        unsigned int i0 = pi[base+0];
        unsigned int i1 = pi[base+1];
        unsigned int i2 = pi[base+2];
        std::array<std::pair<unsigned int,unsigned int>,3> edges = {{{i0,i1},{i1,i2},{i2,i0}}};
        for (auto e : edges) {
            unsigned int a = std::min(e.first, e.second);
            unsigned int b = std::max(e.first, e.second);
            EdgeKey k{a,b};
            edgeCount[k] = edgeCount[k] + 1;
        }
    }

    // Border edges are those with count == 1
    std::vector<std::pair<unsigned int,unsigned int>> borderEdges;
    for (const auto& kv : edgeCount) {
        if (kv.second == 1) borderEdges.push_back({kv.first.a, kv.first.b});
    }

    if (borderEdges.empty()) {
        LOG_ERROR("buildFaceFromTriangles: no border edges (maybe selection is closed manifold)");
        return nullFace;
    }

    // Build adjacency for border loop ordering
    std::unordered_map<unsigned int, std::vector<unsigned int>> adj;
    for (auto e : borderEdges) {
        adj[e.first].push_back(e.second);
        adj[e.second].push_back(e.first);
    }

    // Find a start vertex (degree==1 preferred)
    unsigned int startV = borderEdges[0].first;
    for (const auto& kv : adj) { if (kv.second.size() == 1) { startV = kv.first; break; } }

    // Walk loop(s) — only handle first outer loop
    std::vector<unsigned int> loop;
    loop.push_back(startV);
    unsigned int cur = startV;
    unsigned int prev = std::numeric_limits<unsigned int>::max();
    while (true) {
        const auto& nbrs = adj[cur];
        unsigned int next = std::numeric_limits<unsigned int>::max();
        for (unsigned int n : nbrs) if (n != prev) { next = n; break; }
        if (next == std::numeric_limits<unsigned int>::max()) break;
        if (next == startV) break;
        loop.push_back(next);
        prev = cur; cur = next;
        if (loop.size() > borderEdges.size()+2) break; // avoid infinite loops
    }

    if (loop.size() < 3) {
        LOG_ERROR("buildFaceFromTriangles: extracted loop too small: {}", loop.size());
        return nullFace;
    }

    // Build OCCT polygon wire
    BRepBuilderAPI_MakePolygon poly;
    for (unsigned int vi : loop) {
        const QVector3D& p = pv[vi];
        poly.Add(gp_Pnt(p.x(), p.y(), p.z()));
    }
    poly.Close();
    if (!poly.IsDone()) {
        LOG_ERROR("buildFaceFromTriangles: polygon construction failed");
        return nullFace;
    }

    BRepBuilderAPI_MakeFace faceMaker(poly.Wire());
    if (!faceMaker.IsDone()) {
        LOG_ERROR("buildFaceFromTriangles: MakeFace failed");
        return nullFace;
    }

    TopoDS_Face f = TopoDS::Face(faceMaker.Face());
    LOG_DEBUG("buildFaceFromTriangles: built face with loop size {} triCount={}", loop.size(), triIndices.size());
    return f;
}
#endif

} // namespace elcad

