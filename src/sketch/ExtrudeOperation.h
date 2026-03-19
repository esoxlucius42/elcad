#pragma once
#include "ui/ExtrudeDialog.h"
#include <QString>

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Shape.hxx>
#endif

namespace elcad {

class Sketch;
class Document;

struct ExtrudeResult {
    bool    success{false};
    QString errorMsg;
#ifdef ELCAD_HAVE_OCCT
    TopoDS_Shape shape;
#endif
};

// Performs OCCT BRepPrimAPI_MakePrism extrusion.
// Returns the resulting solid shape.
class ExtrudeOperation {
public:
#ifdef ELCAD_HAVE_OCCT
    // Extrude face along direction by params.distance.
    // If symmetric: extrudes half+half in both directions.
    static ExtrudeResult extrude(const Sketch& sketch,
                                 const ExtrudeParams& params);

    // Boolean union of extruded shape into target
    static ExtrudeResult booleanAdd(const TopoDS_Shape& base,
                                    const TopoDS_Shape& tool);

    // Boolean subtract of extruded shape from target
    static ExtrudeResult booleanCut(const TopoDS_Shape& base,
                                    const TopoDS_Shape& tool);
#endif
};

} // namespace elcad
