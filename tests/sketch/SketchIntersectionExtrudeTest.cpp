#include "SketchIntersectionFixtures.h"

#include "sketch/ExtrudeOperation.h"
#include "sketch/SketchToWire.h"

#include <sstream>

#ifdef ELCAD_HAVE_OCCT
#include <BRepGProp.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <GProp_GProps.hxx>
#endif

namespace elcad::test {
namespace {

std::string describeBoundarySegments(const std::vector<SketchBoundarySegment>& segments)
{
    std::ostringstream stream;
    stream << "segments=" << segments.size();
    for (const auto& segment : segments) {
        stream << " [type=" << static_cast<int>(segment.type)
               << " start=(" << segment.start.x() << "," << segment.start.y() << ")"
               << " end=(" << segment.end.x() << "," << segment.end.y() << ")"
               << " center=(" << segment.center.x() << "," << segment.center.y() << ")"
               << " radius=" << segment.radius
               << " ccw=" << (segment.counterClockwise ? "true" : "false")
               << " source=" << segment.sourceEntityId << "]";
    }
    return stream.str();
}

#ifdef ELCAD_HAVE_OCCT
double faceArea(const TopoDS_Face& face)
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    return props.Mass();
}

double solidVolume(const TopoDS_Shape& shape)
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return props.Mass();
}
#endif

void verifyResolvedProfile(const Sketch& sketch,
                           const char* label,
                           int regionIndex,
                           std::size_t expectedHoleCount,
                           std::optional<double> expectedArea = std::nullopt,
                           std::optional<float> expectedArcSweepDegrees = std::nullopt)
{
    const std::string prefix = std::string(label) + ": ";
    require(regionIndex >= 0, prefix + "probe point should resolve to a selectable bounded region");

    SketchFaceSelection selection;
    selection.sketchId = sketch.id();
    selection.loopIndices = {regionIndex};

    QString errorMsg;
    const auto profiles = SketchPicker::resolveSelectedProfiles(sketch, selection, &errorMsg);
    require(errorMsg.isEmpty(), prefix + "resolved profile should not report an error");
    require(profiles.size() == 1, prefix + "exactly one selected bounded region should resolve to one profile");

    const auto& profile = profiles.front();
    require(profile.loopIndex == regionIndex, prefix + "resolved profile should preserve the bounded-region index");
    require(profile.isClosed, prefix + "resolved profile should be marked closed");
    require(!profile.boundarySegments.empty(), prefix + "resolved profile should contain outer boundary geometry");
    require(profile.holes.size() == expectedHoleCount, prefix + "resolved profile should preserve expected hole boundaries");
    if (expectedArcSweepDegrees.has_value()) {
        auto normalizeAngle = [](float angle) {
            float normalized = std::fmod(angle, 360.0f);
            if (normalized < 0.0f)
                normalized += 360.0f;
            return normalized;
        };

        const auto hasExpectedArcSweep = std::any_of(
            profile.boundarySegments.begin(),
            profile.boundarySegments.end(),
            [&](const SketchBoundarySegment& boundary) {
                if (boundary.type != SketchEntity::Arc)
                    return false;

                const float startAngle = normalizeAngle(
                    qRadiansToDegrees(std::atan2(boundary.start.y() - boundary.center.y(),
                                                 boundary.start.x() - boundary.center.x())));
                const float endAngle = normalizeAngle(
                    qRadiansToDegrees(std::atan2(boundary.end.y() - boundary.center.y(),
                                                 boundary.end.x() - boundary.center.x())));
                const float sweep = boundary.counterClockwise
                    ? normalizeAngle(endAngle - startAngle)
                    : normalizeAngle(startAngle - endAngle);
                return std::abs(sweep - *expectedArcSweepDegrees) <= 2.0f;
            });
        require(hasExpectedArcSweep, prefix + "resolved profile should preserve the expected arc sweep");
    }

#ifdef ELCAD_HAVE_OCCT
    const auto faceResult = SketchToWire::buildFaceForProfile(profile);
    require(faceResult.success, prefix + "resolved profile should build a planar OCCT face: " +
                                    faceResult.errorMsg.toStdString() + " " +
                                    describeBoundarySegments(profile.boundarySegments));
    require(!faceResult.face.IsNull(), prefix + "resolved profile should build a non-null OCCT face");
    require(BRepCheck_Analyzer(faceResult.face).IsValid(),
            prefix + "resolved profile should build a topologically valid OCCT face");
    if (expectedArea.has_value()) {
        requireNear(static_cast<float>(faceArea(faceResult.face)),
                    static_cast<float>(*expectedArea),
                    3.0f,
                    prefix + "resolved profile should preserve expected planar face area");
    }

    ExtrudeParams params;
    params.distance = 10.0;
    params.mode = 0;
    params.symmetric = false;

    const auto batchResult = ExtrudeOperation::extrudeProfiles(profiles, params);
    require(batchResult.success, prefix + "resolved profile extrusion should succeed");
    require(batchResult.solids.size() == 1, prefix + "resolved profile extrusion should emit exactly one solid");
    require(BRepCheck_Analyzer(batchResult.solids.front()).IsValid(),
            prefix + "resolved profile extrusion should produce a topologically valid solid");
    if (expectedArea.has_value()) {
        requireNear(static_cast<float>(solidVolume(batchResult.solids.front())),
                    static_cast<float>(*expectedArea * params.distance),
                    30.0f,
                    prefix + "resolved profile extrusion should preserve expected prism volume");
    }
#endif
}

void verifyUnselectedRegionsRemainUnchanged(const Sketch& sketch,
                                            const std::vector<int>& selectedRegions,
                                            int unselectedRegion)
{
    SketchFaceSelection selection;
    selection.sketchId = sketch.id();
    selection.loopIndices = selectedRegions;

    QString errorMsg;
    const auto profiles = SketchPicker::resolveSelectedProfiles(sketch, selection, &errorMsg);
    require(errorMsg.isEmpty(), "selected subset should resolve without an error");
    require(profiles.size() == selectedRegions.size(),
            "subset extrusion should resolve exactly the requested profile count");
    require(std::none_of(profiles.begin(),
                         profiles.end(),
                         [&](const SelectedSketchProfile& profile) {
                             return profile.loopIndex == unselectedRegion;
                         }),
            "subset extrusion should not include unselected regions");

#ifdef ELCAD_HAVE_OCCT
    ExtrudeParams params;
    params.distance = 10.0;
    params.mode = 0;
    params.symmetric = false;

    const auto batchResult = ExtrudeOperation::extrudeProfiles(profiles, params);
    require(batchResult.success, "subset extrusion should succeed");
    require(batchResult.solids.size() == selectedRegions.size(),
            "subset extrusion should emit one solid per selected region only");

    SketchFaceSelection remainingSelection;
    remainingSelection.sketchId = sketch.id();
    remainingSelection.loopIndices = {unselectedRegion};

    QString remainingError;
    const auto remainingProfiles =
        SketchPicker::resolveSelectedProfiles(sketch, remainingSelection, &remainingError);
    require(remainingError.isEmpty(), "unselected region should remain resolvable after subset extrusion");
    require(remainingProfiles.size() == 1,
            "unselected region should remain available as its own single profile");
#endif
}

} // namespace

void runSketchIntersectionExtrudeTests()
{
    auto overlapSketch = makeRectangleCircleOverlapSketch();
    const auto overlapTopology = deriveBoundedRegions(*overlapSketch);

    verifyResolvedProfile(*overlapSketch,
                          "overlap-right-rectangle-region",
                          findSelectableRegionIndex(overlapTopology, QVector2D(15.0f, 5.0f)),
                          0);
    verifyResolvedProfile(*overlapSketch,
                          "overlap-intersection-region",
                          findSelectableRegionIndex(overlapTopology, QVector2D(3.0f, 3.0f)),
                          0);
    verifyResolvedProfile(*overlapSketch,
                          "overlap-outer-circle-region",
                          findSelectableRegionIndex(overlapTopology, QVector2D(-5.0f, 5.0f)),
                          0);

    auto nestedSketch = makeNestedRectangleCircleSketch();
    const auto nestedTopology = deriveBoundedRegions(*nestedSketch);
    require(nestedTopology.size() == 2, "nested rectangle-circle sketch should derive outer and inner regions");

    const int annulusRegion = findSelectableRegionIndex(nestedTopology, QVector2D(25.0f, 15.0f));
    require(annulusRegion >= 0,
            "annulus region should remain selectable; " + describeBoundedRegions(nestedTopology));
    verifyResolvedProfile(*nestedSketch,
                          "nested-annulus-region",
                          annulusRegion,
                          1,
                          900.0 - (M_PI * 36.0));

    auto arcSketch = makeClockwiseArcSegmentSketch();
    const auto arcTopology = deriveBoundedRegions(*arcSketch);
    const int arcRegion = findSelectableRegionIndex(arcTopology, QVector2D(0.0f, 0.0f));
    require(arcRegion >= 0,
            "clockwise arc segment region should remain selectable; " + describeBoundedRegions(arcTopology));
    verifyResolvedProfile(*arcSketch,
                          "clockwise-major-arc-region",
                          arcRegion,
                          0,
                          50.0 * ((4.0 * M_PI / 3.0) + (std::sqrt(3.0) * 0.5)),
                          240.0f);

    verifyUnselectedRegionsRemainUnchanged(*overlapSketch,
                                           {findSelectableRegionIndex(overlapTopology, QVector2D(15.0f, 5.0f)),
                                            findSelectableRegionIndex(overlapTopology, QVector2D(-5.0f, 5.0f))},
                                           findSelectableRegionIndex(overlapTopology, QVector2D(3.0f, 3.0f)));
}

} // namespace elcad::test
