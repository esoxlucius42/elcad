#include "sketch/SnapEngine.h"
#include "core/Logger.h"
#include "sketch/Sketch.h"
#include <cmath>

namespace elcad {

SnapResult SnapEngine::snap(QVector2D rawPos, const Sketch* sketch,
                             float snapThresholdMM) const
{
    SnapResult result;
    result.pos = rawPos;

    // ── Vertex snap (higher priority than grid) ───────────────────────────────
    if (m_snapVertex && sketch) {
        float bestDist = snapThresholdMM;
        for (const auto& ep : sketch->entities()) {
            auto tryPoint = [&](QVector2D pt) {
                float d = (pt - rawPos).length();
                if (d < bestDist) {
                    bestDist    = d;
                    result.pos  = pt;
                    result.type = SnapResult::Vertex;
                }
            };

            if (ep->type == SketchEntity::Line) {
                tryPoint(ep->p0.toVec());
                tryPoint(ep->p1.toVec());
                tryPoint((ep->p0.toVec() + ep->p1.toVec()) * 0.5f);  // midpoint
            } else if (ep->type == SketchEntity::Circle ||
                       ep->type == SketchEntity::Arc) {
                tryPoint(ep->p0.toVec()); // center
            }
        }
        if (result.type == SnapResult::Vertex) {
            LOG_TRACE("Snap: vertex hit at ({:.3f},{:.3f})", result.pos.x(), result.pos.y());
            return result;
        }
    }

    // ── Grid snap ─────────────────────────────────────────────────────────────
    if (m_snapGrid && m_gridSize > 0.f) {
        float x = std::round(rawPos.x() / m_gridSize) * m_gridSize;
        float y = std::round(rawPos.y() / m_gridSize) * m_gridSize;
        result.pos  = {x, y};
        result.type = SnapResult::Grid;
        LOG_TRACE("Snap: grid at ({:.3f},{:.3f})", x, y);
    }

    return result;
}

} // namespace elcad
