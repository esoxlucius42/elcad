#pragma once
#include "sketch/SketchEntity.h"
#include <QVector2D>

namespace elcad {

class Sketch;

struct SnapResult {
    enum Type { None, Grid, Vertex, Midpoint };
    QVector2D pos;
    Type      type{None};
};

// Snapping logic for the sketcher.
class SnapEngine {
public:
    SnapEngine() = default;

    void setGridSize(float mm) { m_gridSize = mm; }
    float gridSize() const { return m_gridSize; }

    void setSnapToGrid(bool on)   { m_snapGrid   = on; }
    void setSnapToVertex(bool on) { m_snapVertex = on; }

    bool snapEnabled() const { return m_snapGrid || m_snapVertex; }
    void setSnapEnabled(bool on) { m_snapGrid = on; m_snapVertex = on; }

    // Snap rawPos to nearest grid/vertex.
    // sketch may be nullptr (grid-only snap).
    // snapThresholdMM: world-space distance for vertex snap.
    SnapResult snap(QVector2D rawPos, const Sketch* sketch,
                    float snapThresholdMM = 3.f) const;

private:
    float m_gridSize{10.f};  // 10 mm grid
    bool  m_snapGrid{true};
    bool  m_snapVertex{true};
};

} // namespace elcad
