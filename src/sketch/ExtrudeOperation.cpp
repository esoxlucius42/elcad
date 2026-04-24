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
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomLProp_SLProps.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <TopAbs_Orientation.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Trsf.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>

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

// Extrude an existing TopoDS_Face directly. Uses face triangulation (if available) to
// estimate a normal for extrusion. Returns an error if the face has no triangulation.
ExtrudeResult ExtrudeOperation::extrudeFace(const TopoDS_Face& face, const ExtrudeParams& params)
{
    ExtrudeResult result;

    LOG_INFO("ExtrudeOperation::extrudeFace — distance={:.4f} mode={} symmetric={}",
             params.distance, params.mode, params.symmetric);

    try {
        // Derive the outward face normal from the OCCT surface geometry,
        // honouring face orientation.  This is more reliable than estimating
        // from triangle winding order, which depends on tessellation details.
        BRepAdaptor_Surface adapt(face);
        double uMid = (adapt.FirstUParameter() + adapt.LastUParameter()) * 0.5;
        double vMid = (adapt.FirstVParameter() + adapt.LastVParameter()) * 0.5;
        GeomLProp_SLProps props(adapt.Surface().Surface(), uMid, vMid, 1, 1e-6);

        gp_Vec n;
        if (props.IsNormalDefined()) {
            gp_Dir dir = props.Normal();
            if (face.Orientation() == TopAbs_REVERSED)
                dir.Reverse();
            n = gp_Vec(dir);
        } else {
            // Fallback: estimate from triangulation if surface normal unavailable
            TopLoc_Location loc;
            Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
            if (tri.IsNull()) {
                result.errorMsg = "Face has no triangulation and surface normal is undefined.";
                LOG_ERROR("extrudeFace failed — no triangulation and surface normal undefined");
                return result;
            }
            Standard_Integer n1, n2, n3;
            tri->Triangle(1).Get(n1, n2, n3);
            gp_Pnt p1 = tri->Node(n1).Transformed(loc);
            gp_Pnt p2 = tri->Node(n2).Transformed(loc);
            gp_Pnt p3 = tri->Node(n3).Transformed(loc);
            gp_Vec e1(p1, p2), e2(p1, p3);
            n = e1.Crossed(e2);
            double len = n.Magnitude();
            if (len < 1e-12) {
                result.errorMsg = "Could not determine face normal (degenerate triangulation).";
                LOG_ERROR("extrudeFace failed — degenerate triangulation with near-zero normal");
                return result;
            }
            n /= len;
        }

        double d = params.distance;
        if (params.symmetric) {
            gp_Vec back(-n.X() * d * 0.5, -n.Y() * d * 0.5, -n.Z() * d * 0.5);
            gp_Trsf trsf;
            trsf.SetTranslation(back);
            BRepBuilderAPI_Transform mover(face, trsf, true);
            BRepPrimAPI_MakePrism prism(mover.Shape(), gp_Vec(n.X() * d, n.Y() * d, n.Z() * d));
            if (!prism.IsDone()) {
                result.errorMsg = "BRepPrimAPI_MakePrism failed (extrudeFace symmetric)";
                LOG_ERROR("extrudeFace (symmetric) failed — prism::IsDone returned false");
                return result;
            }
            result.shape = prism.Shape();
        } else {
            gp_Vec vec(n.X() * d, n.Y() * d, n.Z() * d);
            BRepPrimAPI_MakePrism prism(face, vec);
            if (!prism.IsDone()) {
                result.errorMsg = "BRepPrimAPI_MakePrism failed (extrudeFace)";
                LOG_ERROR("extrudeFace failed — prism::IsDone returned false");
                return result;
            }
            result.shape = prism.Shape();
        }

        BRepCheck_Analyzer check(result.shape);
        if (!check.IsValid()) {
            result.errorMsg = "Extruded shape is invalid (topology error)";
            LOG_ERROR("extrudeFace failed — BRepCheck_Analyzer reports invalid topology");
            result.shape = TopoDS_Shape();
            return result;
        }

        result.success = true;
        LOG_INFO("extrudeFace succeeded — distance={:.4f} symmetric={}", d, params.symmetric);
    } catch (const Standard_Failure& e) {
        result.errorMsg = QString("OCCT exception: %1").arg(e.GetMessageString());
        LOG_ERROR("extrudeFace threw OCCT exception: '{}'", e.GetMessageString());
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
        ShapeUpgrade_UnifySameDomain unify(fuse.Shape(), /*unifyFaces=*/true,
                                                          /*unifyEdges=*/true,
                                                          /*concatBSplines=*/false);
        unify.Build();
        result.shape   = unify.Shape();
        result.success = true;
        LOG_INFO("Boolean union succeeded — coplanar faces merged");
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
        ShapeUpgrade_UnifySameDomain unify(cut.Shape(), /*unifyFaces=*/true,
                                                         /*unifyEdges=*/true,
                                                         /*concatBSplines=*/false);
        unify.Build();
        result.shape   = unify.Shape();
        result.success = true;
        LOG_INFO("Boolean cut succeeded — coplanar faces merged");
    } catch (const Standard_Failure& e) {
        result.errorMsg = QString("OCCT exception: %1").arg(e.GetMessageString());
        LOG_ERROR("Boolean cut threw OCCT exception: '{}'", e.GetMessageString());
    }
    return result;
}

} // namespace elcad

#endif // ELCAD_HAVE_OCCT
