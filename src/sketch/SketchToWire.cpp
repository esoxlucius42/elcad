#include "sketch/SketchToWire.h"

#ifdef ELCAD_HAVE_OCCT

#include "core/Logger.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"
#include "sketch/SketchEntity.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
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
#include <ShapeFix_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRepAlgo_NormalProjection.hxx>
#include <gp_Pln.hxx>
#include <QtMath>
#include <cmath>

namespace elcad {

static gp_Pnt toGp(QVector2D p2d, const SketchPlane& plane) {
    QVector3D p = plane.to3D(p2d);
    return gp_Pnt(p.x(), p.y(), p.z());
}

static gp_Circ makeGpCircle(QVector2D center, float radius, const SketchPlane& plane)
{
    gp_Pnt circleCenter = toGp(center, plane);
    gp_Dir norm(plane.normal().x(), plane.normal().y(), plane.normal().z());
    gp_Dir xd(plane.xAxis().x(), plane.xAxis().y(), plane.xAxis().z());
    return gp_Circ(gp_Ax2(circleCenter, norm, xd), radius);
}

static gp_Pln makeGpPlane(const SketchPlane& plane)
{
    const QVector3D origin = plane.origin();
    const QVector3D normal = plane.normal();
    const QVector3D xAxis = plane.xAxis();
    return gp_Pln(gp_Ax3(gp_Pnt(origin.x(), origin.y(), origin.z()),
                         gp_Dir(normal.x(), normal.y(), normal.z()),
                         gp_Dir(xAxis.x(), xAxis.y(), xAxis.z())));
}

static double normalizeRadians(double angle)
{
    angle = std::fmod(angle, 2.0 * M_PI);
    if (angle < 0.0)
        angle += 2.0 * M_PI;
    return angle;
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

static SketchToWireResult buildWireFromSegments(const std::vector<SketchBoundarySegment>& segments,
                                                const SketchPlane& plane,
                                                bool requireClosedFace,
                                                const QString& emptyError,
                                                const QString& disconnectedError,
                                                const QString& openWireError,
                                                const char* logContext)
{
    SketchToWireResult result;
    if (segments.empty()) {
        result.errorMsg = emptyError;
        LOG_ERROR("{} failed — no boundary segments were provided", logContext);
        return result;
    }

    LOG_DEBUG("{}: converting {} resolved boundary segments", logContext, segments.size());

    BRepBuilderAPI_MakeWire wireBuilder;
    bool hasEdges = false;
    TopTools_ListOfShape edges;
    std::vector<QVector2D> vertexPositions;
    vertexPositions.reserve(segments.size());
    std::vector<TopoDS_Vertex> sharedVertices;
    sharedVertices.reserve(segments.size());
    for (const auto& segment : segments)
        vertexPositions.push_back(segment.start);
    for (const auto& point : vertexPositions)
        sharedVertices.push_back(BRepBuilderAPI_MakeVertex(toGp(point, plane)).Vertex());

    for (std::size_t index = 0; index < segments.size(); ++index) {
        const auto& segment = segments[index];
        const QVector2D& startPoint = vertexPositions[index];
        const QVector2D& endPoint = vertexPositions[(index + 1) % vertexPositions.size()];
        const TopoDS_Vertex& startVertex = sharedVertices[index];
        const TopoDS_Vertex& endVertex = sharedVertices[(index + 1) % sharedVertices.size()];
        try {
            if (segment.type == SketchEntity::Line) {
                gp_Pnt p0 = toGp(startPoint, plane);
                gp_Pnt p1 = toGp(endPoint, plane);
                if (p0.Distance(p1) < 1e-6) {
                    LOG_WARN("{}: skipping degenerate boundary line from entity {}",
                             logContext, segment.sourceEntityId);
                    continue;
                }
                Handle(Geom_TrimmedCurve) edge = GC_MakeSegment(p0, p1).Value();
                edges.Append(BRepBuilderAPI_MakeEdge(edge, startVertex, endVertex).Edge());
                hasEdges = true;
            } else if (segment.type == SketchEntity::Arc) {
                gp_Pnt start = toGp(startPoint, plane);
                gp_Pnt end = toGp(endPoint, plane);
                if (start.Distance(end) < 1e-6) {
                    LOG_WARN("{}: skipping degenerate boundary arc from entity {}",
                             logContext, segment.sourceEntityId);
                    continue;
                }

                const double startAngle =
                    normalizeRadians(std::atan2(startPoint.y() - segment.center.y(),
                                                startPoint.x() - segment.center.x()));
                const double endAngle =
                    normalizeRadians(std::atan2(endPoint.y() - segment.center.y(),
                                                endPoint.x() - segment.center.x()));
                const double sweep = segment.counterClockwise
                    ? normalizeRadians(endAngle - startAngle)
                    : -normalizeRadians(startAngle - endAngle);
                const double midAngle = startAngle + (sweep * 0.5);
                const QVector2D midPoint(
                    segment.center.x() + segment.radius * std::cos(midAngle),
                    segment.center.y() + segment.radius * std::sin(midAngle));

                Handle(Geom_TrimmedCurve) edge =
                    GC_MakeArcOfCircle(start, toGp(midPoint, plane), end).Value();
                edges.Append(BRepBuilderAPI_MakeEdge(edge, startVertex, endVertex).Edge());
                hasEdges = true;
            } else if (segment.type == SketchEntity::Circle) {
                gp_Circ circle = makeGpCircle(segment.center, segment.radius, plane);
                Handle(Geom_TrimmedCurve) edge =
                    GC_MakeArcOfCircle(circle, 0.0, 2.0 * M_PI, true).Value();
                edges.Append(BRepBuilderAPI_MakeEdge(edge).Edge());
                hasEdges = true;
            }
        } catch (...) {
            result.errorMsg = "Edge construction failed for selected boundary geometry.";
            LOG_WARN("{}: failed to build resolved boundary segment from entity {} type={} "
                     "start=({:.4f},{:.4f}) end=({:.4f},{:.4f}) radius={:.4f} ccw={}",
                     logContext,
                     segment.sourceEntityId,
                     static_cast<int>(segment.type),
                     segment.start.x(),
                     segment.start.y(),
                     segment.end.x(),
                     segment.end.y(),
                     segment.radius,
                     segment.counterClockwise);
            return result;
        }
    }

    if (!hasEdges) {
        result.errorMsg = emptyError;
        LOG_ERROR("{} failed — all resolved boundary segments were degenerate", logContext);
        return result;
    }

    wireBuilder.Add(edges);
    if (!wireBuilder.IsDone()) {
        result.errorMsg = disconnectedError;
        LOG_ERROR("{} failed — resolved boundary segments could not be connected into a wire",
                  logContext);
        return result;
    }

    ShapeFix_Wire fixer;
    fixer.SetMaxTolerance(0.1);
    fixer.Load(wireBuilder.Wire());
    fixer.FixReorder();
    fixer.FixConnected();
    fixer.FixClosed();

    result.wire = fixer.Wire();

    BRepBuilderAPI_MakeFace faceMaker(makeGpPlane(plane), result.wire, /*Inside=*/true);
    if (faceMaker.IsDone()) {
        result.face = faceMaker.Face();
        result.success = true;
        LOG_DEBUG("{}: closed wire from resolved boundary segments built successfully", logContext);
    } else {
        result.errorMsg = openWireError;
        if (requireClosedFace) {
            LOG_WARN("{}: resolved boundary wire is open (MakeFace failed)", logContext);
            return result;
        }

        result.success = true;
        LOG_WARN("{}: resolved boundary wire is open (MakeFace failed)", logContext);
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
    SketchToWireResult result = buildWireFromSegments(profile.boundarySegments,
                                                      profile.plane,
                                                      true,
                                                      QString("Selected sketch face %1 has no usable boundary geometry.")
                                                          .arg(profile.loopIndex),
                                                      QString("Selected sketch face %1 could not be connected into a closed wire.")
                                                          .arg(profile.loopIndex),
                                                      QString("Selected sketch face %1 is not a closed loop.")
                                                          .arg(profile.loopIndex),
                                                      "SketchToWire::buildFaceForProfile");
    if (!result.success || result.wire.IsNull())
        return result;

    BRepBuilderAPI_MakeFace faceMaker(makeGpPlane(profile.plane), result.wire, /*Inside=*/true);
    if (!faceMaker.IsDone()) {
        result.success = false;
        result.errorMsg =
            QString("Selected sketch face %1 could not initialize a planar face.")
                .arg(profile.loopIndex);
        return result;
    }

    for (const auto& hole : profile.holes) {
        SketchToWireResult holeResult = buildWireFromSegments(
            hole.boundarySegments,
            profile.plane,
            true,
            QString("Selected sketch hole %1 for face %2 has no usable boundary geometry.")
                .arg(hole.holeLoopIndex)
                .arg(profile.loopIndex),
            QString("Selected sketch hole %1 for face %2 could not be connected into a closed wire.")
                .arg(hole.holeLoopIndex)
                .arg(profile.loopIndex),
            QString("Selected sketch hole %1 for face %2 is not a closed loop.")
                .arg(hole.holeLoopIndex)
                .arg(profile.loopIndex),
            "SketchToWire::buildFaceForProfile::hole");
        if (!holeResult.success || holeResult.wire.IsNull()) {
            holeResult.success = false;
            return holeResult;
        }

        faceMaker.Add(holeResult.wire);
    }

    if (!faceMaker.IsDone()) {
        result.success = false;
        result.errorMsg =
            QString("Selected sketch face %1 could not include all hole boundaries.")
                .arg(profile.loopIndex);
        return result;
    }

    ShapeFix_Face fixer(faceMaker.Face());
    fixer.FixOrientation();
    fixer.Perform();
    result.face = fixer.Face();
    result.success = !result.face.IsNull();
    if (!result.success) {
        result.errorMsg =
            QString("Selected sketch face %1 produced an invalid face after applying hole boundaries.")
                .arg(profile.loopIndex);
    }
    return result;
}

} // namespace elcad

#endif // ELCAD_HAVE_OCCT
