#include "document/TransformOps.h"
#include "core/Logger.h"

#ifdef ELCAD_HAVE_OCCT
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <gp_Trsf.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

namespace elcad {

static TopoDS_Shape applyTrsf(const TopoDS_Shape& shape, const gp_Trsf& trsf)
{
    BRepBuilderAPI_Transform builder(shape, trsf, /*copy=*/true);
    if (!builder.IsDone()) {
        LOG_ERROR("TransformOps: BRepBuilderAPI_Transform::IsDone() returned false — "
                  "shape type={} null={} — returning original shape unchanged",
                  static_cast<int>(shape.ShapeType()), shape.IsNull() ? "true" : "false");
        return shape;
    }
    return builder.Shape();
}

TopoDS_Shape TransformOps::translate(const TopoDS_Shape& shape,
                                      double dx, double dy, double dz)
{
    LOG_DEBUG("TransformOps::translate — dx={:.4f} dy={:.4f} dz={:.4f}", dx, dy, dz);
    gp_Trsf t;
    t.SetTranslation(gp_Vec(dx, dy, dz));
    return applyTrsf(shape, t);
}

TopoDS_Shape TransformOps::rotate(const TopoDS_Shape& shape,
                                   double ax, double ay, double az,
                                   double ox, double oy, double oz,
                                   double angleDeg)
{
    LOG_DEBUG("TransformOps::rotate — axis=({:.3f},{:.3f},{:.3f}) "
              "origin=({:.3f},{:.3f},{:.3f}) angle={:.3f}°",
              ax, ay, az, ox, oy, oz, angleDeg);
    gp_Trsf t;
    gp_Ax1  axis(gp_Pnt(ox, oy, oz), gp_Dir(ax, ay, az));
    t.SetRotation(axis, angleDeg * M_PI / 180.0);
    return applyTrsf(shape, t);
}

TopoDS_Shape TransformOps::scale(const TopoDS_Shape& shape,
                                  double ox, double oy, double oz,
                                  double factor)
{
    LOG_DEBUG("TransformOps::scale — origin=({:.3f},{:.3f},{:.3f}) factor={:.4f}",
              ox, oy, oz, factor);
    if (factor <= 0.0)
        LOG_WARN("TransformOps::scale — non-positive scale factor {:.4f} "
                 "will produce an invalid or mirrored shape", factor);
    gp_Trsf t;
    t.SetScale(gp_Pnt(ox, oy, oz), factor);
    return applyTrsf(shape, t);
}

TopoDS_Shape TransformOps::mirror(const TopoDS_Shape& shape, int planeId)
{
    static const char* planeNames[] = {"XZ (flip Y)", "XY (flip Z)", "YZ (flip X)"};
    LOG_DEBUG("TransformOps::mirror — plane={} ({})",
              planeId, (planeId >= 0 && planeId <= 2) ? planeNames[planeId] : "unknown");
    gp_Trsf t;
    gp_Ax2 mirrorPlane;
    switch (planeId) {
    case 0: // XZ → flip Y
        mirrorPlane = gp_Ax2(gp_Pnt(0,0,0), gp_Dir(0,1,0), gp_Dir(1,0,0));
        break;
    case 1: // XY → flip Z
        mirrorPlane = gp_Ax2(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0));
        break;
    case 2: // YZ → flip X
        mirrorPlane = gp_Ax2(gp_Pnt(0,0,0), gp_Dir(1,0,0), gp_Dir(0,1,0));
        break;
    default:
        LOG_WARN("TransformOps::mirror — unknown planeId={}, returning original shape", planeId);
        return shape;
    }
    t.SetMirror(mirrorPlane);
    return applyTrsf(shape, t);
}

bool TransformOps::isValid(const TopoDS_Shape& shape)
{
    BRepCheck_Analyzer check(shape);
    bool valid = check.IsValid();
    if (!valid)
        LOG_WARN("TransformOps::isValid — BRepCheck_Analyzer reports INVALID topology "
                 "— shape type={} null={} — the shape may have self-intersections, "
                 "degenerate faces, or disconnected wires",
                 static_cast<int>(shape.ShapeType()), shape.IsNull() ? "true" : "false");
    return valid;
}

} // namespace elcad

#endif // ELCAD_HAVE_OCCT
