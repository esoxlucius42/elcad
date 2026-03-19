#pragma once
#include <QString>

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#endif

namespace elcad {

class Sketch;

// Result of converting a Sketch to OCCT geometry.
struct SketchToWireResult {
    bool    success{false};
    QString errorMsg;
#ifdef ELCAD_HAVE_OCCT
    TopoDS_Wire wire;
    TopoDS_Face face;
#endif
};

// Converts all non-construction SketchEntity lines/circles/arcs in a Sketch
// into an OCCT wire (and face for extrusion).
// Entities are expected to form a closed loop; open loops produce a wire only.
class SketchToWire {
public:
#ifdef ELCAD_HAVE_OCCT
    static SketchToWireResult convert(const Sketch& sketch);
#endif
};

} // namespace elcad
