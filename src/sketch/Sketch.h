#pragma once
#include "sketch/SketchPlane.h"
#include "sketch/SketchEntity.h"
#include "sketch/SketchConstraint.h"
#include <QObject>
#include <vector>
#include <memory>

namespace elcad {

// Owns a set of 2D entities and constraints on a sketch plane.
class Sketch : public QObject {
    Q_OBJECT
public:
    explicit Sketch(SketchPlane plane, QObject* parent = nullptr);

    quint64 id() const { return m_id; }

    const SketchPlane& plane() const { return m_plane; }

    // Entity creation helpers
    SketchEntity* addLine(float x0, float y0, float x1, float y1, bool construction = false);
    SketchEntity* addCircle(float cx, float cy, float r);
    SketchEntity* addArc(float cx, float cy, float r, float a0, float a1);

    void          removeEntity(quint64 id);
    SketchEntity* entityById(quint64 id) const;

    const std::vector<std::unique_ptr<SketchEntity>>& entities() const { return m_entities; }

    // Constraint management
    SketchConstraint* addConstraint(SketchConstraint::Type type,
                                    quint64 entity1, quint64 entity2 = 0,
                                    float value = 0.f);
    void              removeConstraint(quint64 id);

    const std::vector<std::unique_ptr<SketchConstraint>>& constraints() const { return m_constraints; }

    // Selection
    void clearSelection();

signals:
    void entityAdded(SketchEntity* e);
    void entityRemoved(quint64 id);
    void entityChanged(SketchEntity* e);
    void sketchChanged();

private:
    quint64      m_id;
    SketchPlane  m_plane;
    std::vector<std::unique_ptr<SketchEntity>>    m_entities;
    std::vector<std::unique_ptr<SketchConstraint>> m_constraints;
};

} // namespace elcad
