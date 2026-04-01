#include "sketch/SketchPicker.h"
#include "sketch/SketchPlane.h"
#include "sketch/SketchEntity.h"
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace elcad {

// ── Internal helpers ──────────────────────────────────────────────────────────

static float pointSegDistSq(QVector2D p, QVector2D a, QVector2D b)
{
    QVector2D ab = b - a;
    float lenSq = QVector2D::dotProduct(ab, ab);
    if (lenSq < 1e-10f) return (p - a).lengthSquared();
    float t = QVector2D::dotProduct(p - a, ab) / lenSq;
    t = qBound(0.0f, t, 1.0f);
    return (p - (a + ab * t)).lengthSquared();
}

static float pointArcDist(QVector2D p, QVector2D center, float r,
                           float startDeg, float endDeg)
{
    QVector2D d    = p - center;
    float     dist = d.length();
    if (dist < 1e-6f) return r;

    float angleDeg = qRadiansToDegrees(std::atan2(d.y(), d.x()));
    if (angleDeg < 0.f) angleDeg += 360.f;

    bool full = (qAbs(endDeg - startDeg - 360.f) < 1e-3f ||
                 qAbs(endDeg - startDeg)          < 1e-3f);

    bool inRange = full;
    if (!full) {
        float s = startDeg, e = endDeg;
        inRange = (s <= e) ? (angleDeg >= s && angleDeg <= e)
                            : (angleDeg >= s || angleDeg <= e);
    }

    if (inRange) return qAbs(dist - r);

    // Closest to one of the arc's endpoints.
    float a0r = qDegreesToRadians(startDeg);
    float a1r = qDegreesToRadians(endDeg);
    QVector2D ep0 = center + QVector2D(r * qCos(a0r), r * qSin(a0r));
    QVector2D ep1 = center + QVector2D(r * qCos(a1r), r * qSin(a1r));
    return qMin((p - ep0).length(), (p - ep1).length());
}

// ── SketchPicker::pick ────────────────────────────────────────────────────────

SketchHit SketchPicker::pick(const QVector3D& rayOrigin,
                              const QVector3D& rayDir,
                              Document*        doc)
{
    if (!doc) return {};

    SketchHit best;
    float bestDistSq   = 1e30f;
    int   bestPriority = -1;  // 2=point endpoint, 1=line/arc edge, 0=filled area

    for (auto& sketchPtr : doc->sketches()) {
        const Sketch&      sketch = *sketchPtr;
        const SketchPlane& plane  = sketch.plane();

        float t = plane.intersectRay(rayOrigin, rayDir);
        if (t < 0.f) continue;

        QVector3D hit3d = rayOrigin + rayDir * t;
        QVector2D hit2d = plane.to2D(hit3d);

        // ── Entity hits ──────────────────────────────────────────────────────
        for (const auto& ep : sketch.entities()) {
            const SketchEntity& e = *ep;
            if (e.construction) continue;

            if (e.type == SketchEntity::Line) {
                QVector2D p0 = e.p0.toVec(), p1 = e.p1.toVec();

                // Endpoint proximity (priority 2)
                float d0 = (hit2d - p0).length();
                float d1 = (hit2d - p1).length();
                float dPt = qMin(d0, d1);
                if (dPt < kPointThreshold) {
                    float dSq = dPt * dPt;
                    if (bestPriority < 2 || dSq < bestDistSq) {
                        Document::SelectedItem item;
                        item.type     = Document::SelectedItem::Type::SketchPoint;
                        item.sketchId = sketch.id();
                        item.entityId = e.id;
                        item.index    = (d0 <= d1) ? 0 : 1;
                        best          = {sketchPtr.get(), item};
                        bestDistSq    = dSq;
                        bestPriority  = 2;
                    }
                }

                // Line edge (priority 1)
                float lineDSq = pointSegDistSq(hit2d, p0, p1);
                if (std::sqrt(lineDSq) < kLineThreshold) {
                    if (bestPriority < 1 || (bestPriority == 1 && lineDSq < bestDistSq)) {
                        Document::SelectedItem item;
                        item.type     = Document::SelectedItem::Type::SketchLine;
                        item.sketchId = sketch.id();
                        item.entityId = e.id;
                        best          = {sketchPtr.get(), item};
                        bestDistSq    = lineDSq;
                        bestPriority  = 1;
                    }
                }

            } else if (e.type == SketchEntity::Circle) {
                // Circumference (priority 1)
                float d = pointArcDist(hit2d, e.p0.toVec(), e.radius, 0.f, 360.f);
                if (d < kLineThreshold) {
                    float dSq = d * d;
                    if (bestPriority < 1 || (bestPriority == 1 && dSq < bestDistSq)) {
                        Document::SelectedItem item;
                        item.type     = Document::SelectedItem::Type::SketchLine;
                        item.sketchId = sketch.id();
                        item.entityId = e.id;
                        best          = {sketchPtr.get(), item};
                        bestDistSq    = dSq;
                        bestPriority  = 1;
                    }
                }
                // Interior (area, priority 0)
                float distToCenter = (hit2d - e.p0.toVec()).length();
                if (distToCenter < e.radius && bestPriority < 0) {
                    Document::SelectedItem item;
                    item.type     = Document::SelectedItem::Type::SketchArea;
                    item.sketchId = sketch.id();
                    item.entityId = e.id;  // circle entity IS the area
                    item.index    = -1;    // -1 = circle area (not a polygon loop)
                    best          = {sketchPtr.get(), item};
                    bestDistSq    = distToCenter * distToCenter;
                    bestPriority  = 0;
                }

            } else if (e.type == SketchEntity::Arc) {
                // Arc edge (priority 1)
                float d = pointArcDist(hit2d, e.p0.toVec(), e.radius,
                                        e.startAngle, e.endAngle);
                if (d < kLineThreshold) {
                    float dSq = d * d;
                    if (bestPriority < 1 || (bestPriority == 1 && dSq < bestDistSq)) {
                        Document::SelectedItem item;
                        item.type     = Document::SelectedItem::Type::SketchLine;
                        item.sketchId = sketch.id();
                        item.entityId = e.id;
                        best          = {sketchPtr.get(), item};
                        bestDistSq    = dSq;
                        bestPriority  = 1;
                    }
                }
                // Arc endpoints (priority 2)
                float a0r = qDegreesToRadians(e.startAngle);
                float a1r = qDegreesToRadians(e.endAngle);
                QVector2D ep0 = e.p0.toVec() + QVector2D(e.radius * qCos(a0r), e.radius * qSin(a0r));
                QVector2D ep1 = e.p0.toVec() + QVector2D(e.radius * qCos(a1r), e.radius * qSin(a1r));
                float de0 = (hit2d - ep0).length();
                float de1 = (hit2d - ep1).length();
                float dEp = qMin(de0, de1);
                if (dEp < kPointThreshold) {
                    float dSq = dEp * dEp;
                    if (bestPriority < 2 || dSq < bestDistSq) {
                        Document::SelectedItem item;
                        item.type     = Document::SelectedItem::Type::SketchPoint;
                        item.sketchId = sketch.id();
                        item.entityId = e.id;
                        item.index    = (de0 <= de1) ? 0 : 1;
                        best          = {sketchPtr.get(), item};
                        bestDistSq    = dSq;
                        bestPriority  = 2;
                    }
                }
            }
        }

        // ── Polygon area hits (closed loops, priority 0) ──────────────────────
        if (bestPriority < 1) {
            auto loops = findClosedLoops(sketch);
            for (int i = 0; i < static_cast<int>(loops.size()); ++i) {
                if (pointInPolygon(hit2d, loops[i].polygon)) {
                    if (bestPriority < 0) {
                        Document::SelectedItem item;
                        item.type     = Document::SelectedItem::Type::SketchArea;
                        item.sketchId = sketch.id();
                        item.entityId = 0;
                        item.index    = i;
                        best          = {sketchPtr.get(), item};
                        bestDistSq    = 0.f;
                        bestPriority  = 0;
                    }
                    break;
                }
            }
        }
    }

    return best;
}

// ── SketchPicker::findClosedLoops ─────────────────────────────────────────────

std::vector<SketchPicker::Loop> SketchPicker::findClosedLoops(const Sketch& sketch,
                                                               float snapTol)
{
    const auto& entities = sketch.entities();

    // Collect non-construction line segments only.
    struct Seg {
        QVector2D a, b;
        quint64   id;
    };
    std::vector<Seg> segs;
    for (const auto& ep : entities) {
        if (!ep->construction && ep->type == SketchEntity::Line)
            segs.push_back({ep->p0.toVec(), ep->p1.toVec(), ep->id});
    }
    if (segs.size() < 3) return {};

    // Assign integer node IDs to unique endpoints (snapped within tolerance).
    std::vector<QVector2D> nodes;
    auto nodeOf = [&](QVector2D p) -> int {
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
            if ((nodes[i] - p).lengthSquared() < snapTol * snapTol)
                return i;
        nodes.push_back(p);
        return static_cast<int>(nodes.size()) - 1;
    };

    struct Edge {
        int     a, b;
        quint64 id;
    };
    std::vector<Edge> edges;
    edges.reserve(segs.size());
    for (const auto& s : segs) {
        int na = nodeOf(s.a);
        int nb = nodeOf(s.b);
        if (na != nb)
            edges.push_back({na, nb, s.id});
    }

    int N = static_cast<int>(nodes.size());
    // Build adjacency: node → list of (neighbour, edge id)
    std::vector<std::vector<std::pair<int, quint64>>> adj(N);
    for (const auto& e : edges) {
        adj[e.a].push_back({e.b, e.id});
        adj[e.b].push_back({e.a, e.id});
    }

    // Walk closed chains: start from any node with degree 2 that hasn't been visited.
    std::vector<bool> visited(N, false);
    std::vector<Loop> result;

    for (int start = 0; start < N; ++start) {
        if (visited[start] || adj[start].size() != 2) continue;

        // Follow the unique path until we return to start or get stuck.
        std::vector<int>     chain;
        std::vector<quint64> chainEntities;
        int cur  = start;
        int prev = -1;
        bool closed = false;

        while (true) {
            chain.push_back(cur);
            if (static_cast<int>(chain.size()) > N) break;  // safety: no loops longer than N

            // Pick the neighbour that isn't where we came from.
            int     next   = -1;
            quint64 nextId = 0;
            for (auto& [nb, eid] : adj[cur]) {
                if (nb != prev) { next = nb; nextId = eid; break; }
            }
            if (next == -1) break;

            chainEntities.push_back(nextId);

            if (next == start) { closed = true; break; }

            prev = cur;
            cur  = next;
        }

        if (closed && chain.size() >= 3) {
            Loop loop;
            loop.entityIds = chainEntities;
            for (int n : chain) {
                loop.polygon.push_back(nodes[n]);
                visited[n] = true;
            }
            result.push_back(std::move(loop));
        }
    }

    return result;
}

// ── SketchPicker::pointInPolygon ──────────────────────────────────────────────

bool SketchPicker::pointInPolygon(QVector2D pt, const std::vector<QVector2D>& poly)
{
    int n = static_cast<int>(poly.size());
    if (n < 3) return false;

    int crossings = 0;
    for (int i = 0; i < n; ++i) {
        QVector2D a = poly[i];
        QVector2D b = poly[(i + 1) % n];
        if ((a.y() <= pt.y() && b.y() > pt.y()) ||
            (b.y() <= pt.y() && a.y() > pt.y()))
        {
            float xIntersect = a.x() + (pt.y() - a.y()) / (b.y() - a.y()) * (b.x() - a.x());
            if (pt.x() < xIntersect)
                ++crossings;
        }
    }
    return (crossings % 2) == 1;
}

} // namespace elcad
