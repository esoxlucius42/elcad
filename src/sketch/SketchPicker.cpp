#include "sketch/SketchPicker.h"
#include "core/Logger.h"
#include "sketch/SketchPlane.h"
#include "sketch/SketchEntity.h"
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <limits>

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

static float signedPolygonArea(const std::vector<QVector2D>& poly)
{
    float area = 0.0f;
    const int n = static_cast<int>(poly.size());
    for (int i = 0; i < n; ++i) {
        const QVector2D a = poly[i];
        const QVector2D b = poly[(i + 1) % n];
        area += a.x() * b.y() - b.x() * a.y();
    }
    return area * 0.5f;
}

static float polygonArea(const std::vector<QVector2D>& poly)
{
    return std::abs(signedPolygonArea(poly));
}

static bool pointsAlmostEqual(QVector2D a, QVector2D b, float tol)
{
    return (a - b).lengthSquared() <= tol * tol;
}

static bool sameBoundaryGeometry(const SketchBoundarySegment& lhs,
                                 const SketchBoundarySegment& rhs,
                                 float tolerance)
{
    if (lhs.type != rhs.type)
        return false;

    const bool sameDirection =
        pointsAlmostEqual(lhs.start, rhs.start, tolerance) &&
        pointsAlmostEqual(lhs.end, rhs.end, tolerance);
    const bool reversedDirection =
        pointsAlmostEqual(lhs.start, rhs.end, tolerance) &&
        pointsAlmostEqual(lhs.end, rhs.start, tolerance);
    if (!sameDirection && !reversedDirection)
        return false;

    if (lhs.type == SketchEntity::Arc) {
        return pointsAlmostEqual(lhs.center, rhs.center, tolerance) &&
               std::abs(lhs.radius - rhs.radius) <= tolerance;
    }

    return true;
}

static SketchBoundarySegment makeLineBoundary(QVector2D start,
                                              QVector2D end,
                                              quint64 sourceEntityId)
{
    SketchBoundarySegment boundary;
    boundary.type = SketchEntity::Line;
    boundary.sourceEntityId = sourceEntityId;
    boundary.start = start;
    boundary.end = end;
    return boundary;
}

static SketchBoundarySegment makeArcBoundary(QVector2D center,
                                             float radius,
                                             QVector2D start,
                                             QVector2D end,
                                             quint64 sourceEntityId,
                                             bool counterClockwise)
{
    SketchBoundarySegment boundary;
    boundary.type = SketchEntity::Arc;
    boundary.sourceEntityId = sourceEntityId;
    boundary.start = start;
    boundary.end = end;
    boundary.center = center;
    boundary.radius = radius;
    boundary.counterClockwise = counterClockwise;
    return boundary;
}

static SketchBoundarySegment reverseBoundary(const SketchBoundarySegment& boundary)
{
    SketchBoundarySegment reversed = boundary;
    std::swap(reversed.start, reversed.end);
    if (reversed.type == SketchEntity::Arc)
        reversed.counterClockwise = !reversed.counterClockwise;
    return reversed;
}

static bool canMergeLineBoundaries(const SketchBoundarySegment& lhs,
                                   const SketchBoundarySegment& rhs,
                                   float tolerance)
{
    const QVector2D lhsDir = lhs.end - lhs.start;
    const QVector2D rhsDir = rhs.end - rhs.start;
    if (lhsDir.lengthSquared() <= tolerance * tolerance ||
        rhsDir.lengthSquared() <= tolerance * tolerance) {
        return false;
    }

    const float cross = lhsDir.x() * rhsDir.y() - lhsDir.y() * rhsDir.x();
    if (std::abs(cross) > tolerance * (lhsDir.length() + rhsDir.length()))
        return false;

    if (QVector2D::dotProduct(lhsDir, rhsDir) <= 0.0f)
        return false;

    const QVector2D rhsStartOffset = rhs.start - lhs.start;
    const float lhsOffsetCross = lhsDir.x() * rhsStartOffset.y() - lhsDir.y() * rhsStartOffset.x();
    return std::abs(lhsOffsetCross) <= tolerance * lhsDir.length();
}

static bool canMergeArcBoundaries(const SketchBoundarySegment& lhs,
                                  const SketchBoundarySegment& rhs,
                                  float tolerance)
{
    return pointsAlmostEqual(lhs.center, rhs.center, tolerance) &&
           std::abs(lhs.radius - rhs.radius) <= tolerance &&
           lhs.counterClockwise == rhs.counterClockwise;
}

static std::vector<SketchBoundarySegment> mergeBoundarySegments(
    const std::vector<SketchBoundarySegment>& segments,
    float tolerance = 1e-3f)
{
    std::vector<SketchBoundarySegment> merged;
    merged.reserve(segments.size());

    for (const auto& segment : segments) {
        if (merged.empty()) {
            merged.push_back(segment);
            continue;
        }

        auto& current = merged.back();
        const bool sameSource = current.sourceEntityId == segment.sourceEntityId;
        const bool contiguous = pointsAlmostEqual(current.end, segment.start, tolerance);
        const bool mergeableLine =
            current.type == SketchEntity::Line &&
            segment.type == SketchEntity::Line &&
            canMergeLineBoundaries(current, segment, tolerance);
        const bool mergeableArc =
            current.type == SketchEntity::Arc &&
            segment.type == SketchEntity::Arc &&
            canMergeArcBoundaries(current, segment, tolerance);

        if (!sameSource || !contiguous || (!mergeableLine && !mergeableArc)) {
            merged.push_back(segment);
            continue;
        }

        current.end = segment.end;
        if (mergeableArc && pointsAlmostEqual(current.start, current.end, tolerance)) {
            current.type = SketchEntity::Circle;
        }
    }

    return merged;
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
            auto  loopTopology = buildLoopTopology(sketch);
            int   bestLoop   = -1;
            float bestArea   = std::numeric_limits<float>::max();
            for (int i = 0; i < static_cast<int>(loopTopology.size()); ++i) {
                if (!selectableLoopContainsPoint(loopTopology, i, hit2d)) continue;
                const float area = polygonArea(loopTopology[i].polygon);
                if (area < bestArea) {
                    bestArea = area;
                    bestLoop = i;
                }
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

    auto normalizeAngle = [](float angle) -> float {
        float normalized = std::fmod(angle, 360.0f);
        if (normalized < 0.0f)
            normalized += 360.0f;
        return normalized;
    };

    auto isFullCircle = [](const RawCircle& circle) -> bool {
        return std::abs(circle.endDeg - circle.startDeg - 360.f) < 1e-3f ||
               std::abs(circle.endDeg - circle.startDeg)          < 1e-3f;
    };

    auto signedSpan = [&](const RawCircle& circle) -> float {
        return isFullCircle(circle) ? 360.0f : (circle.endDeg - circle.startDeg);
    };

    auto boundaryIsCounterClockwise = [&](const RawCircle& circle) -> bool {
        return isFullCircle(circle) || signedSpan(circle) >= 0.0f;
    };

    auto angleProgress = [&](const RawCircle& circle, float angle, float* progressOut) -> bool {
        const float start = normalizeAngle(circle.startDeg);
        const float ang = normalizeAngle(angle);

        if (isFullCircle(circle)) {
            float delta = normalizeAngle(ang - start);
            if (progressOut)
                *progressOut = delta;
            return true;
        }

        const float span = signedSpan(circle);
        if (span > 0.0f) {
            const float delta = normalizeAngle(ang - start);
            if (delta <= span + 0.01f) {
                if (progressOut)
                    *progressOut = delta;
                return true;
            }
            return false;
        }

        const float delta = normalizeAngle(start - ang);
        if (delta <= -span + 0.01f) {
            if (progressOut)
                *progressOut = delta;
            return true;
        }
        return false;
    };

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
            bool  full  = isFullCircle(C);
            for (int s : {-1, 1}) {
                float t = (-bc + s * sqrtD) / (2.f * ac);
                if (t <= tMin || t >= 1.f - tMin) continue;
                QVector2D pt  = L.a + d * t;
                float ang = qRadiansToDegrees(
                    std::atan2(pt.y() - C.center.y(), pt.x() - C.center.x()));
                ang = normalizeAngle(ang);
                // Filter to arc's angular range.
                float progress = 0.0f;
                if (!full && !angleProgress(C, ang, &progress)) continue;
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
        for (int k = 0; k + 1 < static_cast<int>(tv.size()); ++k) {
            const QVector2D start = lines[i].a + ab * tv[k];
            const QVector2D end = lines[i].a + ab * tv[k + 1];
            result.push_back({start,
                              end,
                              lines[i].entityId,
                              makeLineBoundary(start, end, lines[i].entityId)});
        }
    }

    // Circles / Arcs — polygon approximation with extra vertices at intersections
    for (int j = 0; j < static_cast<int>(circles.size()); ++j) {
        const auto& C    = circles[j];
        bool        full = isFullCircle(C);
        const float span = signedSpan(C);
        const bool  counterClockwise = boundaryIsCounterClockwise(C);

        std::vector<float> angles;
        if (full) {
            for (int k = 0; k < kArcSegs; ++k)
                angles.push_back(normalizeAngle(C.startDeg + 360.f * k / kArcSegs));
        } else {
            for (int k = 0; k <= kArcSegs; ++k)
                angles.push_back(normalizeAngle(C.startDeg + span * k / kArcSegs));
        }
        for (float a : angleSplits[j])
            angles.push_back(normalizeAngle(a));

        std::sort(angles.begin(), angles.end(),
                  [&](float lhs, float rhs) {
                      float lhsProgress = 0.0f;
                      float rhsProgress = 0.0f;
                      angleProgress(C, lhs, &lhsProgress);
                      angleProgress(C, rhs, &rhsProgress);
                      return lhsProgress < rhsProgress;
                  });
        angles.erase(std::unique(angles.begin(), angles.end(),
                                 [&](float x, float y){
                                     float xProgress = 0.0f;
                                     float yProgress = 0.0f;
                                     angleProgress(C, x, &xProgress);
                                     angleProgress(C, y, &yProgress);
                                     return std::abs(xProgress - yProgress) < 0.01f;
                                 }),
                     angles.end());

        auto ptAt = [&](float deg) -> QVector2D {
            float rad = qDegreesToRadians(deg);
            return C.center + QVector2D(C.r * std::cos(rad), C.r * std::sin(rad));
        };

        for (int k = 0; k + 1 < static_cast<int>(angles.size()); ++k) {
            const QVector2D start = ptAt(angles[k]);
            const QVector2D end = ptAt(angles[k + 1]);
            result.push_back({start,
                              end,
                              C.entityId,
                              makeArcBoundary(C.center, C.r, start, end, C.entityId, counterClockwise)});
        }
        if (full) { // close the loop
            const QVector2D start = ptAt(angles.back());
            const QVector2D end = ptAt(angles.front());
            result.push_back({start,
                              end,
                              C.entityId,
                              makeArcBoundary(C.center, C.r, start, end, C.entityId, counterClockwise)});
        }
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

    struct UEdge {
        int a, b;
        quint64 entityId;
        SketchBoundarySegment boundary;
    };
    std::vector<UEdge> uedges;
    uedges.reserve(segs.size());
    for (const auto& s : segs) {
        int na = nodeOf(s.a);
        int nb = nodeOf(s.b);
        bool duplicateEdge = false;
        for (const auto& existing : uedges) {
            const bool sameNodes =
                (existing.a == na && existing.b == nb) ||
                (existing.a == nb && existing.b == na);
            if (!sameNodes)
                continue;
            if (sameBoundaryGeometry(existing.boundary, s.boundary, snapTol * 0.5f)) {
                duplicateEdge = true;
                break;
            }
        }

        if (na != nb && !duplicateEdge)
            uedges.push_back({na, nb, s.entityId, s.boundary});
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
        SketchBoundarySegment boundary;
        int     next{-1};
        bool    visited{false};
    };
    std::vector<HalfEdge> he;
    he.reserve(2 * E);
    for (const auto& e : uedges) {
        he.push_back({e.a, e.b, e.entityId, e.boundary});                  // 2i
        he.push_back({e.b, e.a, e.entityId, reverseBoundary(e.boundary)}); // 2i+1
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
            loop.boundarySegments.push_back(he[idx].boundary);
        }
        result.push_back(std::move(loop));
    }

    return result;
}

std::vector<SketchPicker::DerivedLoopTopology> SketchPicker::buildLoopTopology(const Sketch& sketch,
                                                                               float snapTol)
{
    const auto loops = findClosedLoops(sketch, snapTol);
    auto computeInteriorSample = [](const std::vector<QVector2D>& polygon) -> QVector2D {
        if (polygon.empty())
            return {};

        QVector2D average;
        for (const auto& point : polygon)
            average += point;
        average /= static_cast<float>(polygon.size());
        if (SketchPicker::pointInPolygon(average, polygon))
            return average;

        float areaFactor = 0.0f;
        QVector2D areaCentroid;
        for (int i = 0; i < static_cast<int>(polygon.size()); ++i) {
            const QVector2D a = polygon[i];
            const QVector2D b = polygon[(i + 1) % static_cast<int>(polygon.size())];
            const float cross = a.x() * b.y() - b.x() * a.y();
            areaFactor += cross;
            areaCentroid += QVector2D((a.x() + b.x()) * cross,
                                      (a.y() + b.y()) * cross);
        }
        if (std::abs(areaFactor) > 1e-5f) {
            areaCentroid /= (3.0f * areaFactor);
            if (SketchPicker::pointInPolygon(areaCentroid, polygon))
                return areaCentroid;
        }

        for (int i = 0; i < static_cast<int>(polygon.size()); ++i) {
            const QVector2D a = polygon[i];
            const QVector2D b = polygon[(i + 1) % static_cast<int>(polygon.size())];
            const QVector2D edgeMidpoint = (a + b) * 0.5f;
            const QVector2D blendedMidpoint = (average + edgeMidpoint) * 0.5f;
            if (SketchPicker::pointInPolygon(blendedMidpoint, polygon))
                return blendedMidpoint;

            const QVector2D triangleCentroid = (polygon.front() + a + b) / 3.0f;
            if (SketchPicker::pointInPolygon(triangleCentroid, polygon))
                return triangleCentroid;
        }

        return average;
    };

    std::vector<DerivedLoopTopology> topology;
    topology.reserve(loops.size());

    for (int i = 0; i < static_cast<int>(loops.size()); ++i) {
        DerivedLoopTopology derived;
        derived.loopIndex = i;
        derived.polygon = loops[i].polygon;
        derived.entityIds = loops[i].entityIds;
        derived.boundarySegments = loops[i].boundarySegments;
        derived.signedArea = signedPolygonArea(loops[i].polygon);
        derived.samplePoint = computeInteriorSample(derived.polygon);
        topology.push_back(std::move(derived));
    }

    const float kAreaEpsilon = 1e-4f;
    for (int i = 0; i < static_cast<int>(topology.size()); ++i) {
        if (topology[i].polygon.empty())
            continue;

        float bestParentArea = std::numeric_limits<float>::max();

        for (int j = 0; j < static_cast<int>(topology.size()); ++j) {
            if (i == j || topology[j].polygon.empty())
                continue;

            const float candidateArea = polygonArea(topology[j].polygon);
            const float loopArea = polygonArea(topology[i].polygon);
            if (candidateArea <= loopArea + kAreaEpsilon)
                continue;
            if (!pointInPolygon(topology[i].samplePoint, topology[j].polygon))
                continue;

            if (candidateArea < bestParentArea) {
                bestParentArea = candidateArea;
                topology[i].parentLoopIndex = j;
            }
        }
    }

    for (auto& loop : topology)
        loop.childLoopIndices.clear();

    for (int i = 0; i < static_cast<int>(topology.size()); ++i) {
        if (!topology[i].parentLoopIndex.has_value())
            continue;
        topology[*topology[i].parentLoopIndex].childLoopIndices.push_back(i);
    }

    for (int i = 0; i < static_cast<int>(topology.size()); ++i) {
        int depth = 0;
        std::optional<int> parent = topology[i].parentLoopIndex;
        while (parent.has_value()) {
            ++depth;
            parent = topology[*parent].parentLoopIndex;
        }
        topology[i].nestingDepth = depth;
        topology[i].isSelectableMaterial = (depth % 2) == 0;
    }

    return topology;
}

std::vector<SelectedSketchProfile> SketchPicker::resolveSelectedProfiles(
    const Sketch& sketch,
    const SketchFaceSelection& selection,
    QString* errorMsg)
{
    if (errorMsg) errorMsg->clear();

    if (selection.sketchId != sketch.id()) {
        if (errorMsg)
            *errorMsg = "Selected sketch faces do not belong to the current sketch.";
        LOG_WARN("SketchPicker::resolveSelectedProfiles: sketch mismatch (selection={}, sketch={})",
                 selection.sketchId, sketch.id());
        return {};
    }

    if (selection.loopIndices.empty()) {
        if (errorMsg)
            *errorMsg = "Select at least one sketch face to extrude.";
        LOG_WARN("SketchPicker::resolveSelectedProfiles: empty loop selection for sketch {}",
                 sketch.id());
        return {};
    }

    const auto topology = buildLoopTopology(sketch);
    std::vector<SelectedSketchProfile> profiles;
    profiles.reserve(selection.loopIndices.size());

    auto appendLoopSegments =
        [&](const DerivedLoopTopology& loop,
            std::vector<quint64>& entityIds,
            std::vector<SketchBoundarySegment>& boundarySegments,
            int ownerLoopIndex) -> bool
    {
        if (loop.boundarySegments.empty()) {
            if (errorMsg) {
                *errorMsg = QString("Selected sketch face %1 has no usable boundary geometry.")
                                .arg(ownerLoopIndex);
            }
            LOG_WARN("SketchPicker::resolveSelectedProfiles: loop {} has no boundary segments",
                     ownerLoopIndex);
            return false;
        }

        for (const auto& boundary : loop.boundarySegments) {
            if (boundary.type != SketchEntity::Line && boundary.type != SketchEntity::Arc) {
                if (errorMsg) {
                    *errorMsg = QString("Selected sketch face %1 references unsupported boundary geometry.")
                                    .arg(ownerLoopIndex);
                }
                LOG_WARN("SketchPicker::resolveSelectedProfiles: unsupported boundary type {} for loop {}",
                         static_cast<int>(boundary.type), ownerLoopIndex);
                return false;
            }

            boundarySegments.push_back(boundary);

            const quint64 entityId = boundary.sourceEntityId;
            if (std::find(entityIds.begin(), entityIds.end(), entityId) != entityIds.end()) {
                LOG_DEBUG("SketchPicker::resolveSelectedProfiles: ignoring duplicate entity {} for loop {}",
                          entityId, ownerLoopIndex);
                continue;
            }

            const SketchEntity* entity = sketch.entityById(entityId);
            if (!entity || entity->construction) {
                if (errorMsg) {
                    *errorMsg = QString("Selected sketch face %1 references invalid geometry.")
                                    .arg(ownerLoopIndex);
                }
                LOG_WARN("SketchPicker::resolveSelectedProfiles: entity {} missing or construction for loop {}",
                         entityId, ownerLoopIndex);
                return false;
            }

            entityIds.push_back(entityId);
        }

        return true;
    };

    for (int loopIndex : selection.loopIndices) {
        if (loopIndex < 0 || loopIndex >= static_cast<int>(topology.size())) {
            if (errorMsg)
                *errorMsg = QString("Selected sketch face %1 could not be resolved.")
                                .arg(loopIndex);
            LOG_WARN("SketchPicker::resolveSelectedProfiles: loop index {} out of range (loop count={})",
                     loopIndex, topology.size());
            return {};
        }

        const auto& loop = topology[loopIndex];
        if (loop.polygon.size() < 3) {
            if (errorMsg)
                *errorMsg = QString("Selected sketch face %1 is degenerate.").arg(loopIndex);
            LOG_WARN("SketchPicker::resolveSelectedProfiles: loop {} has only {} polygon points",
                     loopIndex, loop.polygon.size());
            return {};
        }
        if (!loop.isSelectableMaterial) {
            if (errorMsg) {
                *errorMsg = QString("Selected sketch face %1 is a hole, not a bounded face. "
                                    "Click the material area outside the hole boundary.")
                                .arg(loopIndex);
            }
            LOG_WARN("SketchPicker::resolveSelectedProfiles: loop {} is nested at odd depth {}",
                     loopIndex, loop.nestingDepth);
            return {};
        }

        SelectedSketchProfile profile;
        profile.sketchId = sketch.id();
        profile.loopIndex = loopIndex;
        profile.plane = sketch.plane();
        profile.polygon = loop.polygon;
        profile.isClosed = true;

        if (!appendLoopSegments(loop,
                                profile.sourceEntityIds,
                                profile.boundarySegments,
                                loopIndex)) {
            return {};
        }

        if (profile.boundarySegments.empty()) {
            if (errorMsg)
                *errorMsg = QString("Selected sketch face %1 has no usable boundary geometry.")
                                .arg(loopIndex);
            LOG_WARN("SketchPicker::resolveSelectedProfiles: loop {} resolved no source entities",
                     loopIndex);
            return {};
        }

        profile.boundarySegments = mergeBoundarySegments(profile.boundarySegments);

        for (int childLoopIndex : loop.childLoopIndices) {
            if (childLoopIndex < 0 || childLoopIndex >= static_cast<int>(topology.size()))
                continue;

            const auto& childLoop = topology[childLoopIndex];
            if (childLoop.isSelectableMaterial)
                continue;
            if (childLoop.polygon.size() < 3) {
                if (errorMsg) {
                    *errorMsg = QString("Selected sketch face %1 contains a degenerate hole boundary.")
                                    .arg(loopIndex);
                }
                LOG_WARN("SketchPicker::resolveSelectedProfiles: child loop {} for parent {} has only {} polygon points",
                         childLoopIndex, loopIndex, childLoop.polygon.size());
                return {};
            }

            HoleBoundary hole;
            hole.holeLoopIndex = childLoopIndex;
            hole.polygon = childLoop.polygon;
            if (!appendLoopSegments(childLoop,
                                    hole.entityIds,
                                    hole.boundarySegments,
                                    loopIndex)) {
                return {};
            }

            if (hole.boundarySegments.empty()) {
                if (errorMsg) {
                    *errorMsg = QString("Selected sketch face %1 contains a hole with no usable boundary geometry.")
                                    .arg(loopIndex);
                }
                LOG_WARN("SketchPicker::resolveSelectedProfiles: child loop {} for parent {} resolved no source entities",
                         childLoopIndex, loopIndex);
                return {};
            }

            hole.boundarySegments = mergeBoundarySegments(hole.boundarySegments);

            profile.holes.push_back(std::move(hole));
        }

        profiles.push_back(std::move(profile));
    }

    LOG_DEBUG("SketchPicker::resolveSelectedProfiles: resolved {} profiles for sketch {}",
              profiles.size(), sketch.id());
    return profiles;
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

bool SketchPicker::loopContainsPoint(const DerivedLoopTopology& loop, QVector2D pt)
{
    return pointInPolygon(pt, loop.polygon);
}

bool SketchPicker::selectableLoopContainsPoint(const std::vector<DerivedLoopTopology>& topology,
                                               int loopIndex,
                                               QVector2D pt)
{
    if (loopIndex < 0 || loopIndex >= static_cast<int>(topology.size()))
        return false;

    const auto& loop = topology[loopIndex];
    if (!loop.isSelectableMaterial)
        return false;
    if (!loopContainsPoint(loop, pt))
        return false;

    for (int childLoopIndex : loop.childLoopIndices) {
        if (childLoopIndex < 0 || childLoopIndex >= static_cast<int>(topology.size()))
            continue;
        if (loopContainsPoint(topology[childLoopIndex], pt))
            return false;
    }

    return true;
}

} // namespace elcad
