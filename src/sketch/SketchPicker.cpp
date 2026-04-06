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
        if (!sketchPtr->visible()) continue;
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
                // Interior (area, priority 0) — now handled by the polygon loop path
                // below, which includes circle/arc approximations in findClosedLoops.

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
        // Includes circle/arc approximation loops.  Pick the smallest-area loop
        // that contains the hit point so that a circle inside a rectangle gets
        // priority over the enclosing rectangle, and a split-circle half gets
        // priority over any larger enclosing region.
        if (bestPriority < 1) {
            auto  loops      = findClosedLoops(sketch);
            int   bestLoop   = -1;
            float bestArea   = std::numeric_limits<float>::max();
            for (int i = 0; i < static_cast<int>(loops.size()); ++i) {
                if (!pointInPolygon(hit2d, loops[i].polygon)) continue;
                // Compute signed area (positive = CCW = bounded face).
                float area = 0.f;
                int   n    = static_cast<int>(loops[i].polygon.size());
                for (int k = 0; k < n; ++k) {
                    QVector2D a = loops[i].polygon[k];
                    QVector2D b = loops[i].polygon[(k + 1) % n];
                    area += a.x() * b.y() - b.x() * a.y();
                }
                area = std::abs(area) * 0.5f;
                if (area < bestArea) { bestArea = area; bestLoop = i; }
            }
            if (bestLoop >= 0 && bestPriority < 0) {
                Document::SelectedItem item;
                item.type     = Document::SelectedItem::Type::SketchArea;
                item.sketchId = sketch.id();
                item.entityId = 0;
                item.index    = bestLoop;
                best          = {sketchPtr.get(), item};
                bestDistSq    = 0.f;
                bestPriority  = 0;
            }
        }
    }

    return best;
}

// ── SketchPicker::flattenSegments ─────────────────────────────────────────────
//
// Returns all non-construction Line / Circle / Arc entities flattened into short
// straight sub-segments.  Circles and arcs are approximated as kArcSegs-gons;
// extra vertices are inserted exactly at every intersection point so that the
// two sub-segments sharing that vertex become graph nodes.
//
// Intersection types handled:
//   • Line × Line  — T-junction (endpoint on interior) and cross-intersection
//   • Line × Circle/Arc — quadratic solve; result filtered to the arc's range

std::vector<SketchPicker::FlatSeg> SketchPicker::flattenSegments(const Sketch& sketch,
                                                                   float snapTol)
{
    constexpr int kArcSegs = 64;  // base polygon approximation per full circle

    // ── Step 1: Collect entities ──────────────────────────────────────────────
    struct RawLine   { QVector2D a, b; quint64 entityId; };
    struct RawCircle { QVector2D center; float r, startDeg, endDeg; quint64 entityId; };

    std::vector<RawLine>   lines;
    std::vector<RawCircle> circles;
    for (const auto& ep : sketch.entities()) {
        if (ep->construction) continue;
        if (ep->type == SketchEntity::Line)
            lines.push_back({ep->p0.toVec(), ep->p1.toVec(), ep->id});
        else if (ep->type == SketchEntity::Circle)
            circles.push_back({ep->p0.toVec(), ep->radius, 0.f, 360.f, ep->id});
        else if (ep->type == SketchEntity::Arc)
            circles.push_back({ep->p0.toVec(), ep->radius,
                               ep->startAngle, ep->endAngle, ep->id});
    }
    if (lines.empty() && circles.empty()) return {};

    // ── Step 2: Collect split t-values (lines) and split angles (circles) ─────
    std::vector<std::vector<float>> tSplits(lines.size(), {0.f, 1.f});
    std::vector<std::vector<float>> angleSplits(circles.size());

    // Returns t ∈ (tMin, 1-tMin) if p lies on the interior of segment [a,b].
    auto tOnSeg = [&](QVector2D p, QVector2D a, QVector2D b) -> float {
        QVector2D ab  = b - a;
        float     len = ab.length();
        if (len < snapTol) return -1.f;
        float t    = QVector2D::dotProduct(p - a, ab) / (len * len);
        float tMin = snapTol / len;
        if (t <= tMin || t >= 1.f - tMin) return -1.f;
        if ((a + ab * t - p).lengthSquared() >= snapTol * snapTol) return -1.f;
        return t;
    };

    // Line × Line
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        for (int j = i + 1; j < static_cast<int>(lines.size()); ++j) {
            float t;
            t = tOnSeg(lines[j].a, lines[i].a, lines[i].b);
            if (t > 0.f) tSplits[i].push_back(t);
            t = tOnSeg(lines[j].b, lines[i].a, lines[i].b);
            if (t > 0.f) tSplits[i].push_back(t);
            t = tOnSeg(lines[i].a, lines[j].a, lines[j].b);
            if (t > 0.f) tSplits[j].push_back(t);
            t = tOnSeg(lines[i].b, lines[j].a, lines[j].b);
            if (t > 0.f) tSplits[j].push_back(t);
            // Cross-intersection (both t-values interior)
            QVector2D p = lines[i].b - lines[i].a;
            QVector2D r = lines[j].b - lines[j].a;
            float denom = p.x() * r.y() - p.y() * r.x();
            if (std::abs(denom) > 1e-10f) {
                QVector2D q = lines[j].a - lines[i].a;
                float tI = (q.x() * r.y() - q.y() * r.x()) / denom;
                float tJ = (q.x() * p.y() - q.y() * p.x()) / denom;
                float minI = (p.length() > snapTol) ? snapTol / p.length() : 1.f;
                float minJ = (r.length() > snapTol) ? snapTol / r.length() : 1.f;
                if (tI > minI && tI < 1.f - minI && tJ > minJ && tJ < 1.f - minJ) {
                    tSplits[i].push_back(tI);
                    tSplits[j].push_back(tJ);
                }
            }
        }
    }

    // Line × Circle/Arc
    for (int li = 0; li < static_cast<int>(lines.size()); ++li) {
        for (int ci = 0; ci < static_cast<int>(circles.size()); ++ci) {
            const RawLine&   L = lines[li];
            const RawCircle& C = circles[ci];
            QVector2D d = L.b - L.a;
            QVector2D f = L.a - C.center;
            float ac  = d.lengthSquared();
            if (ac < 1e-10f) continue;
            float bc   = 2.f * QVector2D::dotProduct(f, d);
            float cc   = f.lengthSquared() - C.r * C.r;
            float disc = bc * bc - 4.f * ac * cc;
            if (disc < 0.f) continue;
            float sqrtD = std::sqrt(std::max(0.f, disc));
            float len   = std::sqrt(ac);
            float tMin  = snapTol / len;
            bool  full  = (std::abs(C.endDeg - C.startDeg - 360.f) < 1e-3f ||
                           std::abs(C.endDeg - C.startDeg)          < 1e-3f);
            for (int s : {-1, 1}) {
                float t = (-bc + s * sqrtD) / (2.f * ac);
                if (t <= tMin || t >= 1.f - tMin) continue;
                QVector2D pt  = L.a + d * t;
                float ang = qRadiansToDegrees(
                    std::atan2(pt.y() - C.center.y(), pt.x() - C.center.x()));
                if (ang < 0.f) ang += 360.f;
                // Filter to arc's angular range.
                if (!full) {
                    float s2 = C.startDeg, e2 = C.endDeg;
                    bool inRange = (s2 <= e2) ? (ang >= s2 - 0.01f && ang <= e2 + 0.01f)
                                              : (ang >= s2 - 0.01f || ang <= e2 + 0.01f);
                    if (!inRange) continue;
                }
                tSplits[li].push_back(t);
                angleSplits[ci].push_back(ang);
            }
        }
    }

    // ── Step 3: Produce flat sub-segments ─────────────────────────────────────
    std::vector<FlatSeg> result;

    // Lines
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        auto& tv = tSplits[i];
        std::sort(tv.begin(), tv.end());
        tv.erase(std::unique(tv.begin(), tv.end(),
                             [](float x, float y){ return std::abs(x - y) < 1e-6f; }),
                 tv.end());
        QVector2D ab = lines[i].b - lines[i].a;
        for (int k = 0; k + 1 < static_cast<int>(tv.size()); ++k)
            result.push_back({lines[i].a + ab * tv[k],
                              lines[i].a + ab * tv[k + 1],
                              lines[i].entityId});
    }

    // Circles / Arcs — polygon approximation with extra vertices at intersections
    for (int j = 0; j < static_cast<int>(circles.size()); ++j) {
        const auto& C    = circles[j];
        bool        full = (std::abs(C.endDeg - C.startDeg - 360.f) < 1e-3f ||
                            std::abs(C.endDeg - C.startDeg)          < 1e-3f);

        std::vector<float> angles;
        if (full) {
            for (int k = 0; k < kArcSegs; ++k)
                angles.push_back(360.f * k / kArcSegs);
        } else {
            float span = C.endDeg - C.startDeg;
            for (int k = 0; k <= kArcSegs; ++k)
                angles.push_back(C.startDeg + span * k / kArcSegs);
        }
        for (float a : angleSplits[j])
            angles.push_back(a);

        std::sort(angles.begin(), angles.end());
        angles.erase(std::unique(angles.begin(), angles.end(),
                                 [](float x, float y){ return std::abs(x - y) < 0.01f; }),
                     angles.end());

        auto ptAt = [&](float deg) -> QVector2D {
            float rad = qDegreesToRadians(deg);
            return C.center + QVector2D(C.r * std::cos(rad), C.r * std::sin(rad));
        };

        for (int k = 0; k + 1 < static_cast<int>(angles.size()); ++k)
            result.push_back({ptAt(angles[k]), ptAt(angles[k + 1]), C.entityId});
        if (full)  // close the loop
            result.push_back({ptAt(angles.back()), ptAt(angles.front()), C.entityId});
    }

    return result;
}

// ── SketchPicker::findClosedLoops ─────────────────────────────────────────────
//
// Finds all minimal bounded faces in the planar arrangement formed by the sketch
// line segments, including sub-regions created by lines that split larger regions.
//
// Algorithm overview:
//   1. Flatten all segments (flattenSegments handles T-junctions and cross-
//      intersections so every touching / crossing point becomes a shared node).
//   2. Build a planar graph: snap endpoints to unique nodes; build edge list.
//   3. Create directed half-edges (each undirected edge → indices 2i and 2i+1).
//   4. At every node, sort outgoing half-edges by angle (CCW).
//   5. Wire next() pointers using the "most-clockwise turn" rule: for half-edge
//      u→v the next half-edge around the left face is the outgoing edge from v
//      that comes just before twin(u→v) in v's CCW-sorted list.
//   6. Trace face cycles by following next() chains.
//   7. Keep only CCW-oriented cycles (positive signed area) = bounded inner faces.

std::vector<SketchPicker::Loop> SketchPicker::findClosedLoops(const Sketch& sketch,
                                                               float snapTol)
{
    auto segs = flattenSegments(sketch, snapTol);
    if (segs.size() < 3) return {};

    // ── Step 2: Build nodes and undirected edges ──────────────────────────────
    std::vector<QVector2D> nodes;
    auto nodeOf = [&](QVector2D p) -> int {
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
            if ((nodes[i] - p).lengthSquared() < snapTol * snapTol)
                return i;
        nodes.push_back(p);
        return static_cast<int>(nodes.size()) - 1;
    };

    struct UEdge { int a, b; quint64 entityId; };
    std::vector<UEdge> uedges;
    uedges.reserve(segs.size());
    for (const auto& s : segs) {
        int na = nodeOf(s.a);
        int nb = nodeOf(s.b);
        if (na != nb)
            uedges.push_back({na, nb, s.entityId});
    }

    int N = static_cast<int>(nodes.size());
    int E = static_cast<int>(uedges.size());
    if (N < 3 || E < 3) return {};

    // ── Step 4: Build directed half-edges ────────────────────────────────────
    // Half-edge 2*i   = forward  (a → b)
    // Half-edge 2*i+1 = backward (b → a)
    // twin(h) = h ^ 1  (flip last bit)
    struct HalfEdge {
        int     from, to;
        quint64 entityId;
        int     next{-1};
        bool    visited{false};
    };
    std::vector<HalfEdge> he;
    he.reserve(2 * E);
    for (const auto& e : uedges) {
        he.push_back({e.a, e.b, e.entityId});   // 2i
        he.push_back({e.b, e.a, e.entityId});   // 2i+1
    }

    // ── Step 5: Angle-sort outgoing half-edges at every node ─────────────────
    std::vector<std::vector<int>> outgoing(N);
    for (int i = 0; i < static_cast<int>(he.size()); ++i)
        outgoing[he[i].from].push_back(i);

    for (int n = 0; n < N; ++n) {
        std::sort(outgoing[n].begin(), outgoing[n].end(), [&](int a, int b) {
            QVector2D da = nodes[he[a].to] - nodes[n];
            QVector2D db = nodes[he[b].to] - nodes[n];
            return std::atan2(da.y(), da.x()) < std::atan2(db.y(), db.x());
        });
    }

    // ── Step 6: Wire next() pointers (most-clockwise turn) ───────────────────
    // For half-edge h = u→v, next(h) is the outgoing half-edge from v that sits
    // just before twin(h) in v's CCW-sorted list (i.e. the clockwise neighbour
    // of the reverse direction), tracing the left-face boundary.
    for (int hi = 0; hi < static_cast<int>(he.size()); ++hi) {
        int v    = he[hi].to;
        int twin = hi ^ 1;  // twin half-edge index

        const auto& out = outgoing[v];
        auto it = std::find(out.begin(), out.end(), twin);
        if (it == out.end()) continue;

        int idx     = static_cast<int>(it - out.begin());
        int prevIdx = (idx - 1 + static_cast<int>(out.size())) % static_cast<int>(out.size());
        he[hi].next = out[prevIdx];
    }

    // ── Step 7 & 8: Trace cycles; keep bounded (CCW) faces ───────────────────
    std::vector<Loop> result;

    for (int start = 0; start < static_cast<int>(he.size()); ++start) {
        if (he[start].visited || he[start].next == -1) continue;

        // Trace the face cycle.
        std::vector<int> cycle;
        int  cur   = start;
        bool valid = true;
        do {
            if (he[cur].visited || he[cur].next == -1) { valid = false; break; }
            he[cur].visited = true;
            cycle.push_back(cur);
            cur = he[cur].next;
            if (static_cast<int>(cycle.size()) > static_cast<int>(he.size()))
                { valid = false; break; }
        } while (cur != start);

        if (!valid || cycle.size() < 3) continue;

        // Signed area: positive = CCW = bounded inner face.
        float area = 0.f;
        for (int idx : cycle) {
            QVector2D a = nodes[he[idx].from];
            QVector2D b = nodes[he[idx].to];
            area += a.x() * b.y() - b.x() * a.y();
        }
        if (area <= 0.f) continue;  // outer / infinite face

        Loop loop;
        for (int idx : cycle) {
            loop.polygon.push_back(nodes[he[idx].from]);
            loop.entityIds.push_back(he[idx].entityId);
        }
        result.push_back(std::move(loop));
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
