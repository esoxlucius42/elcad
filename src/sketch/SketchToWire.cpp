#include "sketch/SketchToWire.h"

#ifdef ELCAD_HAVE_OCCT

#include "core/Logger.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"
#include "sketch/SketchEntity.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <GC_MakeSegment.hxx>
#include <GC_MakeCircle.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAlgo_NormalProjection.hxx>
#include <QtMath>

namespace elcad {

static gp_Pnt toGp(QVector2D p2d, const SketchPlane& plane) {
    QVector3D p = plane.to3D(p2d);
    return gp_Pnt(p.x(), p.y(), p.z());
}

SketchToWireResult SketchToWire::convert(const Sketch& sketch)
{
    SketchToWireResult result;
    const SketchPlane& plane = sketch.plane();

    const auto& entities = sketch.entities();
    int nonConstruction = 0;
    for (const auto& ep : entities)
        if (!ep->construction) ++nonConstruction;

    LOG_DEBUG("SketchToWire: converting sketch — {} total entities, {} non-construction",
              entities.size(), nonConstruction);

    BRepBuilderAPI_MakeWire wireBuilder;
    bool hasEdges = false;

    for (const auto& ep : entities) {
        if (ep->construction) continue;  // skip construction geometry

        try {
            if (ep->type == SketchEntity::Line) {
                gp_Pnt p0 = toGp(ep->p0.toVec(), plane);
                gp_Pnt p1 = toGp(ep->p1.toVec(), plane);
                if (p0.Distance(p1) < 1e-6) {
                    LOG_WARN("SketchToWire: skipping degenerate line (id={}) — "
                             "endpoints are coincident or nearly so: "
                             "p0=({:.4f},{:.4f}) p1=({:.4f},{:.4f})",
                             ep->id, ep->p0.x, ep->p0.y, ep->p1.x, ep->p1.y);
                    continue;
                }
                Handle(Geom_TrimmedCurve) seg = GC_MakeSegment(p0, p1).Value();
                wireBuilder.Add(BRepBuilderAPI_MakeEdge(seg).Edge());
                hasEdges = true;

            } else if (ep->type == SketchEntity::Circle) {
                gp_Pnt center = toGp(ep->p0.toVec(), plane);
                gp_Dir norm(plane.normal().x(), plane.normal().y(), plane.normal().z());
                gp_Dir xd(plane.xAxis().x(), plane.xAxis().y(), plane.xAxis().z());
                gp_Ax2 ax(center, norm, xd);
                gp_Circ circle(ax, ep->radius);
                Handle(Geom_TrimmedCurve) arc =
                    GC_MakeArcOfCircle(circle, 0.0, 2.0 * M_PI, true).Value();
                wireBuilder.Add(BRepBuilderAPI_MakeEdge(arc).Edge());
                hasEdges = true;

            } else if (ep->type == SketchEntity::Arc) {
                gp_Pnt center = toGp(ep->p0.toVec(), plane);
                gp_Dir norm(plane.normal().x(), plane.normal().y(), plane.normal().z());
                gp_Dir xd(plane.xAxis().x(), plane.xAxis().y(), plane.xAxis().z());
                gp_Ax2 ax(center, norm, xd);
                gp_Circ circle(ax, ep->radius);
                double a0 = qDegreesToRadians(ep->startAngle);
                double a1 = qDegreesToRadians(ep->endAngle);
                Handle(Geom_TrimmedCurve) arc =
                    GC_MakeArcOfCircle(circle, a0, a1, true).Value();
                wireBuilder.Add(BRepBuilderAPI_MakeEdge(arc).Edge());
                hasEdges = true;
            }
        } catch (...) {
            result.errorMsg = "Edge construction failed for entity " +
                              QString::number(ep->id);
            // Log entity details to help diagnose the failure
            LOG_WARN("SketchToWire: failed to build edge for entity id={} type={} "
                     "p0=({:.4f},{:.4f}) p1=({:.4f},{:.4f}) radius={:.4f} "
                     "startAngle={:.2f}° endAngle={:.2f}°",
                     ep->id, static_cast<int>(ep->type),
                     ep->p0.x, ep->p0.y, ep->p1.x, ep->p1.y,
                     ep->radius, ep->startAngle, ep->endAngle);
        }
    }

    if (!hasEdges) {
        result.errorMsg = "Sketch has no non-construction geometry";
        LOG_ERROR("SketchToWire failed — no usable edges produced from {} entities "
                  "(all may be construction, degenerate, or failed to build)",
                  entities.size());
        return result;
    }

    if (!wireBuilder.IsDone()) {
        result.errorMsg = "Wire builder failed: edges may not be connected";
        // Dump all non-construction entities to help diagnose connectivity issues
        std::string dump;
        for (const auto& ep : entities) {
            if (ep->construction) continue;
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                "  entity id=%llu type=%d p0=(%.3f,%.3f) p1=(%.3f,%.3f) r=%.3f\n",
                static_cast<unsigned long long>(ep->id), static_cast<int>(ep->type),
                ep->p0.x, ep->p0.y, ep->p1.x, ep->p1.y, ep->radius);
            dump += buf;
        }
        LOG_ERROR("SketchToWire failed — wire builder could not connect edges; "
                  "edges are likely disconnected or have large gaps.\n"
                  "Offending entities ({} non-construction):\n{}",
                  nonConstruction, dump);
        return result;
    }

    // Try to fix the wire (reorder edges, fix orientation)
    ShapeFix_Wire fixer;
    fixer.SetMaxTolerance(0.1);
    fixer.Load(wireBuilder.Wire());
    fixer.FixReorder();
    fixer.FixConnected();
    fixer.FixClosed();

    result.wire = fixer.Wire();

    // Try to build a face (only works for closed wires)
    BRepBuilderAPI_MakeFace faceMaker(result.wire, /*OnlyPlane=*/true);
    if (faceMaker.IsDone()) {
        result.face    = faceMaker.Face();
        result.success = true;
        LOG_DEBUG("SketchToWire: closed wire → face built successfully");
    } else {
        // Open wire — still a valid wire, no face
        result.success  = true;
        result.errorMsg = "Wire is open — extrusion may produce a shell, not a solid";
        LOG_WARN("SketchToWire: wire is open (MakeFace failed) — "
                 "the profile is not a closed loop; extrusion will produce a shell. "
                 "Check that all line/arc endpoints are connected with no gaps.");
    }

    return result;
}

} // namespace elcad

#endif // ELCAD_HAVE_OCCT
