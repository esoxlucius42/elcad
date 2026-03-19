#pragma once
#include <cstdint>

namespace elcad {

// Geometric and dimensional constraint data.
// NOTE: constraint solving (libslvs) is planned for a later phase.
// These structures define the constraint model; they are stored in Sketch
// and will be fed to the solver once it is integrated.
struct SketchConstraint {
    enum Type {
        // Geometric
        Coincident,     // two points coincide
        Parallel,       // two lines parallel
        Perpendicular,  // two lines perpendicular
        Tangent,        // line/circle tangent
        Equal,          // equal length or radius
        Horizontal,     // line is horizontal
        Vertical,       // line is vertical
        Symmetric,      // two points symmetric about a line/axis
        // Dimensional
        Distance,       // distance between two points/entities
        Angle,          // angle between two lines (degrees)
        Radius,         // circle/arc radius
        Fixed,          // entity is fixed in space
    };

    quint64 id{0};
    Type    type;
    quint64 entity1{0};     // primary entity
    quint64 entity2{0};     // secondary entity (0 if unary)
    quint64 point1{0};      // specific point on entity1 (0 = whole entity)
    quint64 point2{0};      // specific point on entity2
    float   value{0.f};     // for dimensional constraints (mm or degrees)
    bool    active{true};

    static quint64 s_nextId;

    SketchConstraint() : id(s_nextId++) {}
    SketchConstraint(Type t) : id(s_nextId++), type(t) {}
};

} // namespace elcad
