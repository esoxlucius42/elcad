#include "sketch/ExtrudeOperation.h"

#ifdef ELCAD_HAVE_OCCT

#include "core/Logger.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"
#include "sketch/SketchToWire.h"

#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Vec.hxx>
#include <gp_Trsf.hxx>
#include <BRepCheck_Analyzer.hxx>

namespace elcad {

ExtrudeResult ExtrudeOperation::extrude(const Sketch& sketch,
                                         const ExtrudeParams& params)
{
    ExtrudeResult result;

    LOG_INFO("ExtrudeOperation::extrude — distance={:.4f} mode={} symmetric={}",
             params.distance, params.mode, params.symmetric);

    SketchToWireResult wire = SketchToWire::convert(sketch);
    if (!wire.success || wire.face.IsNull()) {
        result.errorMsg = wire.errorMsg.isEmpty()
            ? "Could not build closed face from sketch"
            : wire.errorMsg;
        LOG_ERROR("Extrude failed — could not build wire/face from sketch: {} "
                  "— sketch has {} entities",
                  result.errorMsg.toStdString(), sketch.entities().size());
        return result;
    }

    const SketchPlane& plane = sketch.plane();
    QVector3D n = plane.normal();
    double    d = params.distance;

    LOG_DEBUG("Extrude: plane='{}' normal=({:.3f},{:.3f},{:.3f}) distance={:.4f}",
              plane.name().toStdString(), n.x(), n.y(), n.z(), d);

    try {
        if (params.symmetric) {
            // Translate face back by d/2, then extrude by d
            gp_Vec back(-n.x() * d * 0.5,
                        -n.y() * d * 0.5,
                        -n.z() * d * 0.5);
            gp_Trsf trsf;
            trsf.SetTranslation(back);
            BRepBuilderAPI_Transform mover(wire.face, trsf, true);
            BRepPrimAPI_MakePrism prism(mover.Shape(),
                gp_Vec(n.x() * d, n.y() * d, n.z() * d));
            if (!prism.IsDone()) {
                result.errorMsg = "BRepPrimAPI_MakePrism failed (symmetric)";
                LOG_ERROR("Extrude (symmetric) failed — BRepPrimAPI_MakePrism::IsDone() "
                          "returned false — distance={:.4f} normal=({:.3f},{:.3f},{:.3f})",
                          d, n.x(), n.y(), n.z());
                return result;
            }
            result.shape   = prism.Shape();
        } else {
            gp_Vec vec(n.x() * d, n.y() * d, n.z() * d);
            BRepPrimAPI_MakePrism prism(wire.face, vec);
            if (!prism.IsDone()) {
                result.errorMsg = "BRepPrimAPI_MakePrism failed";
                LOG_ERROR("Extrude failed — BRepPrimAPI_MakePrism::IsDone() returned false "
                          "— distance={:.4f} normal=({:.3f},{:.3f},{:.3f})",
                          d, n.x(), n.y(), n.z());
                return result;
            }
            result.shape = prism.Shape();
        }

        BRepCheck_Analyzer check(result.shape);
        if (!check.IsValid()) {
            result.errorMsg = "Extruded shape is invalid (topology error)";
            LOG_ERROR("Extrude failed — BRepCheck_Analyzer reports INVALID topology on "
                      "extruded shape — distance={:.4f} symmetric={} — the profile face "
                      "may have self-intersections or near-zero-length edges",
                      d, params.symmetric);
            result.shape = TopoDS_Shape();
            return result;
        }

        result.success = true;
        LOG_INFO("Extrude succeeded — distance={:.4f} symmetric={} mode={}",
                 d, params.symmetric, params.mode);
    } catch (const Standard_Failure& e) {
        result.errorMsg = QString("OCCT exception: %1").arg(e.GetMessageString());
        LOG_ERROR("Extrude threw OCCT exception: '{}' — distance={:.4f} "
                  "symmetric={} normal=({:.3f},{:.3f},{:.3f}) entities={}",
                  e.GetMessageString(), d, params.symmetric,
                  n.x(), n.y(), n.z(), sketch.entities().size());
    }

    return result;
}

ExtrudeResult ExtrudeOperation::booleanAdd(const TopoDS_Shape& base,
                                            const TopoDS_Shape& tool)
{
    ExtrudeResult result;
    LOG_DEBUG("Boolean union — base type={} tool type={}",
              static_cast<int>(base.ShapeType()), static_cast<int>(tool.ShapeType()));
    try {
        BRepAlgoAPI_Fuse fuse(base, tool);
        fuse.Build();
        if (!fuse.IsDone()) {
            result.errorMsg = "Boolean union failed";
            LOG_ERROR("Boolean union failed — BRepAlgoAPI_Fuse::IsDone() returned false; "
                      "base type={} tool type={} — shapes may be disjoint or have "
                      "incompatible topology",
                      static_cast<int>(base.ShapeType()),
                      static_cast<int>(tool.ShapeType()));
            return result;
        }
        result.shape   = fuse.Shape();
        result.success = true;
        LOG_INFO("Boolean union succeeded");
    } catch (const Standard_Failure& e) {
        result.errorMsg = QString("OCCT exception: %1").arg(e.GetMessageString());
        LOG_ERROR("Boolean union threw OCCT exception: '{}'", e.GetMessageString());
    }
    return result;
}

ExtrudeResult ExtrudeOperation::booleanCut(const TopoDS_Shape& base,
                                            const TopoDS_Shape& tool)
{
    ExtrudeResult result;
    LOG_DEBUG("Boolean cut — base type={} tool type={}",
              static_cast<int>(base.ShapeType()), static_cast<int>(tool.ShapeType()));
    try {
        BRepAlgoAPI_Cut cut(base, tool);
        cut.Build();
        if (!cut.IsDone()) {
            result.errorMsg = "Boolean cut failed";
            LOG_ERROR("Boolean cut failed — BRepAlgoAPI_Cut::IsDone() returned false; "
                      "base type={} tool type={} — the tool may not intersect the base, "
                      "or shapes may have incompatible topology",
                      static_cast<int>(base.ShapeType()),
                      static_cast<int>(tool.ShapeType()));
            return result;
        }
        result.shape   = cut.Shape();
        result.success = true;
        LOG_INFO("Boolean cut succeeded");
    } catch (const Standard_Failure& e) {
        result.errorMsg = QString("OCCT exception: %1").arg(e.GetMessageString());
        LOG_ERROR("Boolean cut threw OCCT exception: '{}'", e.GetMessageString());
    }
    return result;
}

} // namespace elcad

#endif // ELCAD_HAVE_OCCT
