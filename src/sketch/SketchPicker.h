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

    // A closed polygon loop found within a sketch.
    struct Loop {
        std::vector<QVector2D> polygon;    // 2D vertices in order
        std::vector<quint64>   entityIds;  // participating line entity IDs
    };

    // Detect all closed loops of connected non-construction Line entities in a sketch.
    static std::vector<Loop> findClosedLoops(const Sketch& sketch, float snapTol = 0.5f);

    // Ray-casting point-in-polygon test (handles convex and concave polygons).
    static bool pointInPolygon(QVector2D pt, const std::vector<QVector2D>& poly);
};

} // namespace elcad
