#pragma once
#include <QString>

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#endif

namespace elcad {

// All transform operations return a new TopoDS_Shape.
// The caller owns undo/redo via UndoStack.
class TransformOps {
public:
#ifdef ELCAD_HAVE_OCCT
    static TopoDS_Shape translate(const TopoDS_Shape& shape,
                                   double dx, double dy, double dz);

    static TopoDS_Shape rotate(const TopoDS_Shape& shape,
                                double ax, double ay, double az,  // axis direction
                                double ox, double oy, double oz,  // axis origin
                                double angleDeg);

    static TopoDS_Shape scale(const TopoDS_Shape& shape,
                               double ox, double oy, double oz,   // center
                               double factor);

    // Mirror about axis-aligned plane through origin: 0=XZ(flip Y), 1=XY(flip Z), 2=YZ(flip X)
    static TopoDS_Shape mirror(const TopoDS_Shape& shape, int planeId);

    static bool isValid(const TopoDS_Shape& shape);
#endif
};

} // namespace elcad
