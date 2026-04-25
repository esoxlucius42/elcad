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

static SketchToWireResult buildWireFromEntities(const std::vector<SketchEntity>& entities,
                                                const SketchPlane& plane,
                                                bool requireClosedFace,
                                                const QString& emptyError,
                                                const QString& disconnectedError,
                                                const QString& openWireError,
                                                const char* logContext)
{
    SketchToWireResult result;
    int nonConstruction = 0;
    for (const auto& entity : entities)
        if (!entity.construction) ++nonConstruction;

    LOG_DEBUG("{}: converting {} entities ({} non-construction)",
              logContext, entities.size(), nonConstruction);

    BRepBuilderAPI_MakeWire wireBuilder;
    bool hasEdges = false;

    for (const auto& entity : entities) {
        if (entity.construction) continue;

        try {
            if (entity.type == SketchEntity::Line) {
                gp_Pnt p0 = toGp(entity.p0.toVec(), plane);
                gp_Pnt p1 = toGp(entity.p1.toVec(), plane);
                if (p0.Distance(p1) < 1e-6) {
                    LOG_WARN("{}: skipping degenerate line (id={}) — "
                             "endpoints are coincident or nearly so: "
                             "p0=({:.4f},{:.4f}) p1=({:.4f},{:.4f})",
                             logContext, entity.id, entity.p0.x, entity.p0.y, entity.p1.x, entity.p1.y);
                    continue;
                }
                Handle(Geom_TrimmedCurve) seg = GC_MakeSegment(p0, p1).Value();
                wireBuilder.Add(BRepBuilderAPI_MakeEdge(seg).Edge());
                hasEdges = true;

            } else if (entity.type == SketchEntity::Circle) {
                gp_Pnt center = toGp(entity.p0.toVec(), plane);
                gp_Dir norm(plane.normal().x(), plane.normal().y(), plane.normal().z());
                gp_Dir xd(plane.xAxis().x(), plane.xAxis().y(), plane.xAxis().z());
                gp_Ax2 ax(center, norm, xd);
                gp_Circ circle(ax, entity.radius);
                Handle(Geom_TrimmedCurve) arc =
                    GC_MakeArcOfCircle(circle, 0.0, 2.0 * M_PI, true).Value();
                wireBuilder.Add(BRepBuilderAPI_MakeEdge(arc).Edge());
                hasEdges = true;

            } else if (entity.type == SketchEntity::Arc) {
                gp_Pnt center = toGp(entity.p0.toVec(), plane);
                gp_Dir norm(plane.normal().x(), plane.normal().y(), plane.normal().z());
                gp_Dir xd(plane.xAxis().x(), plane.xAxis().y(), plane.xAxis().z());
                gp_Ax2 ax(center, norm, xd);
                gp_Circ circle(ax, entity.radius);
                double a0 = qDegreesToRadians(entity.startAngle);
                double a1 = qDegreesToRadians(entity.endAngle);
                Handle(Geom_TrimmedCurve) arc =
                    GC_MakeArcOfCircle(circle, a0, a1, true).Value();
                wireBuilder.Add(BRepBuilderAPI_MakeEdge(arc).Edge());
                hasEdges = true;
            }
        } catch (...) {
            result.errorMsg = "Edge construction failed for entity " +
                              QString::number(entity.id);
            LOG_WARN("{}: failed to build edge for entity id={} type={} "
                     "p0=({:.4f},{:.4f}) p1=({:.4f},{:.4f}) radius={:.4f} "
                     "startAngle={:.2f}° endAngle={:.2f}°",
                     logContext, entity.id, static_cast<int>(entity.type),
                     entity.p0.x, entity.p0.y, entity.p1.x, entity.p1.y,
                     entity.radius, entity.startAngle, entity.endAngle);
            return result;
        }
    }

    if (!hasEdges) {
        result.errorMsg = emptyError;
        LOG_ERROR("{} failed — no usable edges produced from {} entities "
                  "(all may be construction, degenerate, or failed to build)",
                  logContext, entities.size());
        return result;
    }

    if (!wireBuilder.IsDone()) {
        result.errorMsg = disconnectedError;
        std::string dump;
        for (const auto& entity : entities) {
            if (entity.construction) continue;
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                "  entity id=%llu type=%d p0=(%.3f,%.3f) p1=(%.3f,%.3f) r=%.3f\n",
                static_cast<unsigned long long>(entity.id), static_cast<int>(entity.type),
                entity.p0.x, entity.p0.y, entity.p1.x, entity.p1.y, entity.radius);
            dump += buf;
        }
        LOG_ERROR("{} failed — wire builder could not connect edges; "
                  "edges are likely disconnected or have large gaps.\n"
                  "Offending entities ({} non-construction):\n{}",
                  logContext, nonConstruction, dump);
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

    BRepBuilderAPI_MakeFace faceMaker(result.wire, /*OnlyPlane=*/true);
    if (faceMaker.IsDone()) {
        result.face    = faceMaker.Face();
        result.success = true;
        LOG_DEBUG("{}: closed wire → face built successfully", logContext);
    } else {
        result.errorMsg = openWireError;
        if (requireClosedFace) {
            LOG_WARN("{}: wire is open (MakeFace failed) — selected profile is not a closed loop",
                     logContext);
            return result;
        }

        result.success = true;
        LOG_WARN("{}: wire is open (MakeFace failed) — "
                 "the profile is not a closed loop; extrusion will produce a shell. "
                 "Check that all line/arc endpoints are connected with no gaps.",
                 logContext);
    }

    return result;
}

SketchToWireResult SketchToWire::convert(const Sketch& sketch)
{
    std::vector<SketchEntity> entities;
    entities.reserve(sketch.entities().size());
    for (const auto& entity : sketch.entities())
        entities.push_back(*entity);

    return buildWireFromEntities(entities,
                                 sketch.plane(),
                                 false,
                                 "Sketch has no non-construction geometry",
                                 "Wire builder failed: edges may not be connected",
                                 "Wire is open — extrusion may produce a shell, not a solid",
                                 "SketchToWire");
}

SketchToWireResult SketchToWire::buildFaceForProfile(const SelectedSketchProfile& profile)
{
    return buildWireFromEntities(profile.sourceEntities,
                                 profile.plane,
                                 true,
                                 QString("Selected sketch face %1 has no usable boundary geometry.")
                                     .arg(profile.loopIndex),
                                 QString("Selected sketch face %1 could not be connected into a closed wire.")
                                     .arg(profile.loopIndex),
                                 QString("Selected sketch face %1 is not a closed loop.")
                                     .arg(profile.loopIndex),
                                 "SketchToWire::buildFaceForProfile");
}

} // namespace elcad

#endif // ELCAD_HAVE_OCCT
