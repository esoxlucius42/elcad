#pragma once
#include <QVector2D>
#include <cstdint>

namespace elcad {

// SketchPoint2D — 2D coordinates in sketch plane space (mm)
struct SketchPoint2D {
    float x{0}, y{0};
    SketchPoint2D() = default;
    SketchPoint2D(float x_, float y_) : x(x_), y(y_) {}
    SketchPoint2D(QVector2D v) : x(v.x()), y(v.y()) {}
    QVector2D toVec() const { return {x, y}; }
};

// SketchEntity — a single 2D geometry element on the sketch plane
struct SketchEntity {
    enum Type { Line, Circle, Arc };

    quint64 id{0};
    Type    type{Line};

    // Line / ConstructionLine endpoints; Arc/Circle center stored in p0
    SketchPoint2D p0, p1;

    // Circle / Arc
    float radius{10.f};

    // Arc start/end angles in degrees [0–360)
    float startAngle{0.f};
    float endAngle{360.f};

    bool construction{false};
    bool selected{false};

    static quint64 s_nextId;

    SketchEntity() : id(s_nextId++) {}
    SketchEntity(Type t) : id(s_nextId++), type(t) {}
};

} // namespace elcad
