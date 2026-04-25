#pragma once
#include "sketch/SketchEntity.h"
#include "sketch/SketchPlane.h"
#include <QVector2D>
#include <vector>

namespace elcad {

struct SketchFaceSelection {
    quint64          sketchId{0};
    std::vector<int> loopIndices;
};

struct HoleBoundary {
    int                    holeLoopIndex{-1};
    std::vector<quint64>   entityIds;
    std::vector<SketchEntity> entities;
    std::vector<QVector2D> polygon;
};

struct SelectedSketchProfile {
    quint64                   sketchId{0};
    int                       loopIndex{-1};
    std::vector<quint64>      sourceEntityIds;
    std::vector<SketchEntity> sourceEntities;
    std::vector<QVector2D>    polygon;
    std::vector<HoleBoundary> holes;
    SketchPlane               plane;
    bool                      isClosed{false};
};

} // namespace elcad
