#pragma once
#include "sketch/SketchEntity.h"
#include "sketch/SketchPlane.h"
#include <QVector2D>
#include <vector>

namespace elcad {

using SketchRegionIndex = int;

struct SketchBoundarySegment {
    SketchEntity::Type type{SketchEntity::Line};
    quint64            sourceEntityId{0};
    QVector2D          start;
    QVector2D          end;
    QVector2D          center;
    float              radius{0.0f};
    bool               counterClockwise{true};
};

struct SketchFaceSelection {
    quint64                       sketchId{0};
    std::vector<SketchRegionIndex> loopIndices;
};

struct HoleBoundary {
    SketchRegionIndex                 holeLoopIndex{-1};
    std::vector<quint64>              entityIds;
    std::vector<SketchBoundarySegment> boundarySegments;
    std::vector<QVector2D>            polygon;
};

struct SelectedSketchProfile {
    quint64                           sketchId{0};
    SketchRegionIndex                 loopIndex{-1};
    std::vector<quint64>              sourceEntityIds;
    std::vector<SketchBoundarySegment> boundarySegments;
    std::vector<QVector2D>            polygon;
    std::vector<HoleBoundary>         holes;
    SketchPlane                       plane;
    bool                              isClosed{false};
};

} // namespace elcad
