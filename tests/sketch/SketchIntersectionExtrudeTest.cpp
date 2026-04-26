#include "SketchIntersectionFixtures.h"

#include "sketch/ExtrudeOperation.h"
#include "sketch/SketchToWire.h"

namespace elcad::test {
namespace {

void verifyResolvedProfile(const Sketch& sketch, int regionIndex, std::size_t expectedHoleCount)
{
    require(regionIndex >= 0, "probe point should resolve to a selectable bounded region");

    SketchFaceSelection selection;
    selection.sketchId = sketch.id();
    selection.loopIndices = {regionIndex};

    QString errorMsg;
    const auto profiles = SketchPicker::resolveSelectedProfiles(sketch, selection, &errorMsg);
    require(errorMsg.isEmpty(), "resolved profile should not report an error");
    require(profiles.size() == 1, "exactly one selected bounded region should resolve to one profile");

    const auto& profile = profiles.front();
    require(profile.loopIndex == regionIndex, "resolved profile should preserve the bounded-region index");
    require(profile.isClosed, "resolved profile should be marked closed");
    require(!profile.boundarySegments.empty(), "resolved profile should contain outer boundary geometry");
    require(profile.holes.size() == expectedHoleCount, "resolved profile should preserve expected hole boundaries");

#ifdef ELCAD_HAVE_OCCT
    const auto faceResult = SketchToWire::buildFaceForProfile(profile);
    require(faceResult.success, "resolved profile should build a planar OCCT face");
    require(!faceResult.face.IsNull(), "resolved profile should build a non-null OCCT face");

    ExtrudeParams params;
    params.distance = 10.0;
    params.mode = 0;
    params.symmetric = false;

    const auto batchResult = ExtrudeOperation::extrudeProfiles(profiles, params);
    require(batchResult.success, "resolved profile extrusion should succeed");
    require(batchResult.solids.size() == 1, "resolved profile extrusion should emit exactly one solid");
#endif
}

} // namespace

void runSketchIntersectionExtrudeTests()
{
    auto overlapSketch = makeRectangleCircleOverlapSketch();
    const auto overlapTopology = deriveBoundedRegions(*overlapSketch);

    verifyResolvedProfile(*overlapSketch,
                          findSelectableRegionIndex(overlapTopology, QVector2D(15.0f, 5.0f)),
                          0);
    verifyResolvedProfile(*overlapSketch,
                          findSelectableRegionIndex(overlapTopology, QVector2D(3.0f, 3.0f)),
                          0);
    verifyResolvedProfile(*overlapSketch,
                          findSelectableRegionIndex(overlapTopology, QVector2D(-5.0f, 5.0f)),
                          0);

    auto nestedSketch = makeNestedRectangleCircleSketch();
    const auto nestedTopology = deriveBoundedRegions(*nestedSketch);
    require(nestedTopology.size() == 2, "nested rectangle-circle sketch should derive outer and inner regions");

    const int annulusRegion = findSelectableRegionIndex(nestedTopology, QVector2D(25.0f, 15.0f));
    require(annulusRegion >= 0,
            "annulus region should remain selectable; " + describeBoundedRegions(nestedTopology));
    verifyResolvedProfile(*nestedSketch, annulusRegion, 1);
}

} // namespace elcad::test
