#include "sketch/Sketch.h"
#include "core/Logger.h"
#include "sketch/SketchConstraint.h"
#include <algorithm>

namespace elcad {

quint64 SketchConstraint::s_nextId = 1;

// Helper: readable name for constraint type
static const char* constraintTypeName(SketchConstraint::Type t)
{
    switch (t) {
    case SketchConstraint::Coincident:    return "Coincident";
    case SketchConstraint::Parallel:      return "Parallel";
    case SketchConstraint::Perpendicular: return "Perpendicular";
    case SketchConstraint::Tangent:       return "Tangent";
    case SketchConstraint::Equal:         return "Equal";
    case SketchConstraint::Horizontal:    return "Horizontal";
    case SketchConstraint::Vertical:      return "Vertical";
    case SketchConstraint::Symmetric:     return "Symmetric";
    case SketchConstraint::Distance:      return "Distance";
    case SketchConstraint::Angle:         return "Angle";
    case SketchConstraint::Radius:        return "Radius";
    case SketchConstraint::Fixed:         return "Fixed";
    default:                              return "Unknown";
    }
}

Sketch::Sketch(SketchPlane plane, QObject* parent)
    : QObject(parent)
    , m_plane(std::move(plane))
{}

SketchEntity* Sketch::addLine(float x0, float y0, float x1, float y1, bool construction)
{
    auto e = std::make_unique<SketchEntity>(SketchEntity::Line);
    e->p0 = {x0, y0};
    e->p1 = {x1, y1};
    e->construction = construction;
    SketchEntity* ptr = e.get();
    m_entities.push_back(std::move(e));
    LOG_DEBUG("Sketch: {} line added id={} ({:.3f},{:.3f})→({:.3f},{:.3f})",
              construction ? "construction" : "", ptr->id, x0, y0, x1, y1);
    emit entityAdded(ptr);
    emit sketchChanged();
    return ptr;
}

SketchEntity* Sketch::addCircle(float cx, float cy, float r)
{
    auto e = std::make_unique<SketchEntity>(SketchEntity::Circle);
    e->p0     = {cx, cy};
    e->radius = r;
    SketchEntity* ptr = e.get();
    m_entities.push_back(std::move(e));
    LOG_DEBUG("Sketch: circle added id={} center=({:.3f},{:.3f}) r={:.3f}",
              ptr->id, cx, cy, r);
    emit entityAdded(ptr);
    emit sketchChanged();
    return ptr;
}

SketchEntity* Sketch::addArc(float cx, float cy, float r, float a0, float a1)
{
    auto e = std::make_unique<SketchEntity>(SketchEntity::Arc);
    e->p0         = {cx, cy};
    e->radius     = r;
    e->startAngle = a0;
    e->endAngle   = a1;
    SketchEntity* ptr = e.get();
    m_entities.push_back(std::move(e));
    LOG_DEBUG("Sketch: arc added id={} center=({:.3f},{:.3f}) r={:.3f} angles=[{:.1f}°,{:.1f}°]",
              ptr->id, cx, cy, r, a0, a1);
    emit entityAdded(ptr);
    emit sketchChanged();
    return ptr;
}

void Sketch::removeEntity(quint64 id)
{
    auto it = std::remove_if(m_entities.begin(), m_entities.end(),
        [id](const std::unique_ptr<SketchEntity>& e) { return e->id == id; });
    if (it != m_entities.end()) {
        LOG_DEBUG("Sketch: entity removed id={}", id);
        m_entities.erase(it, m_entities.end());
        emit entityRemoved(id);
        emit sketchChanged();
    }
}

SketchEntity* Sketch::entityById(quint64 id) const
{
    for (auto& e : m_entities)
        if (e->id == id) return e.get();
    return nullptr;
}

SketchConstraint* Sketch::addConstraint(SketchConstraint::Type type,
                                         quint64 entity1, quint64 entity2,
                                         float value)
{
    auto c = std::make_unique<SketchConstraint>(type);
    c->entity1 = entity1;
    c->entity2 = entity2;
    c->value   = value;
    SketchConstraint* ptr = c.get();
    m_constraints.push_back(std::move(c));
    LOG_DEBUG("Sketch: constraint added id={} type={} entity1={} entity2={} value={:.4f}",
              ptr->id, constraintTypeName(type), entity1, entity2, value);
    emit sketchChanged();
    return ptr;
}

void Sketch::removeConstraint(quint64 id)
{
    auto it = std::remove_if(m_constraints.begin(), m_constraints.end(),
        [id](const std::unique_ptr<SketchConstraint>& c) { return c->id == id; });
    if (it != m_constraints.end()) {
        LOG_DEBUG("Sketch: constraint removed id={}", id);
        m_constraints.erase(it, m_constraints.end());
        emit sketchChanged();
    }
}

void Sketch::clearSelection()
{
    for (auto& e : m_entities)
        e->selected = false;
}

} // namespace elcad
