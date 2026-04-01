#pragma once
#include "document/Document.h"
#include "sketch/Sketch.h"
#include <QVector2D>
#include <QVector3D>
#include <vector>

namespace elcad {

// Result of a sketch pick operation.
struct SketchHit {
    Sketch*                sketch{nullptr};  // which completed sketch was hit (not owned)
    Document::SelectedItem item;             // what specifically was hit
    bool valid() const { return sketch != nullptr; }
};

// Stateless utilities for hit-testing completed (non-active) sketches.
class SketchPicker {
public:
    // Maximum 2D distance (mm) to register a point snap hit.
    static constexpr float kPointThreshold = 3.0f;
    // Maximum 2D distance (mm) to register a line/arc/circle hit.
    static constexpr float kLineThreshold  = 2.5f;

    // Find the closest sketch entity near the given world-space ray in any completed sketch.
    // Priority: point endpoints (2) > line/arc edges (1) > filled areas (0).
    static SketchHit pick(const QVector3D& rayOrigin,
                          const QVector3D& rayDir,
                          Document*        doc);

    // A subdivided line segment produced by flattening all intersections.
    struct FlatSeg {
        QVector2D a, b;
        quint64   entityId;  // original sketch entity this sub-segment belongs to
    };

    // Flatten all non-construction Line entities: collect every pairwise
    // intersection (T-junctions where an endpoint lies on another segment AND
    // full cross-intersections where two segments pierce each other), and split
    // every segment at those points.  The original Sketch is unchanged.
    static std::vector<FlatSeg> flattenSegments(const Sketch& sketch, float snapTol = 0.5f);

    // A closed polygon loop found within a sketch.
    struct Loop {
        std::vector<QVector2D> polygon;    // 2D vertices in order
        std::vector<quint64>   entityIds;  // participating line entity IDs
    };

    // Detect all minimal bounded faces in the planar arrangement of the sketch's
    // line segments (uses flattenSegments internally).
    static std::vector<Loop> findClosedLoops(const Sketch& sketch, float snapTol = 0.5f);

    // Ray-casting point-in-polygon test (handles convex and concave polygons).
    static bool pointInPolygon(QVector2D pt, const std::vector<QVector2D>& poly);
};

} // namespace elcad
