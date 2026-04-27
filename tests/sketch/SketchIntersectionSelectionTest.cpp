#include "SketchIntersectionFixtures.h"

#include "sketch/SketchRenderer.h"
#include "viewport/Renderer.h"

#ifdef ELCAD_HAVE_OCCT
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepCheck_Analyzer.hxx>
#endif

#include <cmath>
#include <set>

namespace elcad::test {

namespace {

float triangleArea(QVector2D a, QVector2D b, QVector2D c)
{
    return std::abs(((b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x())) * 0.5f);
}

QVector2D triangleCentroid(QVector2D a, QVector2D b, QVector2D c)
{
    return (a + b + c) / 3.0f;
}

}

void runSketchIntersectionSelectionTests()
{
    auto sketch = makeRectangleCircleOverlapSketch();
    const auto topology = deriveBoundedRegions(*sketch);
    require(topology.size() == 3, "rectangle-circle overlap should derive exactly three bounded regions");

    const int rectangleRegion = findSelectableRegionIndex(topology, QVector2D(15.0f, 5.0f));
    const int overlapRegion = findSelectableRegionIndex(topology, QVector2D(3.0f, 3.0f));
    const int outerCircleRegion = findSelectableRegionIndex(topology, QVector2D(-5.0f, 5.0f));

    const std::string topologySummary = describeBoundedRegions(topology);
    require(rectangleRegion >= 0, "rectangle-only region should be selectable; " + topologySummary);
    require(overlapRegion >= 0, "quarter-circle overlap region should be selectable; " + topologySummary);
    require(outerCircleRegion >= 0, "outer three-quarter circle region should be selectable; " + topologySummary);
    require(std::set<int>{rectangleRegion, overlapRegion, outerCircleRegion}.size() == 3,
            "each probe point should resolve to a distinct bounded region");

    requireNear(absoluteArea(topology[rectangleRegion]),
                400.0f - static_cast<float>(M_PI) * 25.0f,
                2.0f,
                "rectangle-only region should preserve the expected area");
    requireNear(absoluteArea(topology[overlapRegion]),
                static_cast<float>(M_PI) * 25.0f,
                2.0f,
                "quarter-circle overlap region should preserve the expected area");
    requireNear(absoluteArea(topology[outerCircleRegion]),
                static_cast<float>(M_PI) * 75.0f,
                3.0f,
                "outer three-quarter circle region should preserve the expected area");

    const auto rectangleOnlyTriangles = SketchRenderer::triangulatePolygon(topology[rectangleRegion].polygon);
    require(!rectangleOnlyTriangles.empty(),
            "concave rectangle-only region should triangulate for sketch highlight rendering");
    require(rectangleOnlyTriangles.size() % 3 == 0,
            "triangulated sketch highlight should emit complete triangles");

    float triangulatedArea = 0.0f;
    for (std::size_t i = 0; i < rectangleOnlyTriangles.size(); i += 3) {
        const QVector2D a = rectangleOnlyTriangles[i];
        const QVector2D b = rectangleOnlyTriangles[i + 1];
        const QVector2D c = rectangleOnlyTriangles[i + 2];
        triangulatedArea += triangleArea(a, b, c);
        require(SketchPicker::pointInPolygon(triangleCentroid(a, b, c), topology[rectangleRegion].polygon),
                "triangulated sketch highlight should stay inside the selected bounded region");
    }
    requireNear(triangulatedArea,
                absoluteArea(topology[rectangleRegion]),
                2.0f,
                "triangulated sketch highlight should preserve the selected region area");

    auto doc = makeCompletedOverlapDocument();
    require(doc->sketches().size() == 1, "completed overlap document should contain one sketch");
    Sketch* completedSketch = doc->sketches().front().get();
    const auto completedTopology = deriveBoundedRegions(*completedSketch);

    const int completedRectangleRegion = findSelectableRegionIndex(completedTopology, QVector2D(15.0f, 5.0f));
    const int completedOverlapRegion = findSelectableRegionIndex(completedTopology, QVector2D(3.0f, 3.0f));
    const int completedOuterCircleRegion = findSelectableRegionIndex(completedTopology, QVector2D(-5.0f, 5.0f));

    const auto rectangleHit = pickAt(*doc, QVector2D(15.0f, 5.0f));
    require(rectangleHit.valid(), "picker should hit the rectangle-only region");
    require(rectangleHit.item.type == Document::SelectedItem::Type::SketchArea,
            "rectangle-only hit should resolve to a sketch area");
    require(rectangleHit.item.index == completedRectangleRegion,
            "rectangle-only hit should use the resolved bounded-region index");

    const auto overlapHit = pickAt(*doc, QVector2D(3.0f, 3.0f));
    require(overlapHit.valid(), "picker should hit the overlap region");
    require(overlapHit.item.type == Document::SelectedItem::Type::SketchArea,
            "overlap hit should resolve to a sketch area");
    require(overlapHit.item.index == completedOverlapRegion,
            "overlap hit should use the resolved bounded-region index");

    const auto outerCircleHit = pickAt(*doc, QVector2D(-5.0f, 5.0f));
    require(outerCircleHit.valid(), "picker should hit the outer circle region");
    require(outerCircleHit.item.type == Document::SelectedItem::Type::SketchArea,
            "outer circle hit should resolve to a sketch area");
    require(outerCircleHit.item.index == completedOuterCircleRegion,
            "outer circle hit should use the resolved bounded-region index");

    Document::SelectedItem selectedArea;
    selectedArea.type = Document::SelectedItem::Type::SketchArea;
    selectedArea.sketchId = completedSketch->id();
    selectedArea.index = completedRectangleRegion;
    doc->addSelection(selectedArea);
    doc->addSelection(selectedArea);

    const auto faces = doc->selectedSketchFaces(completedSketch->id());
    require(faces.has_value(), "document should resolve selected sketch faces from bounded-region indices");
    require(faces->loopIndices.size() == 1 && faces->loopIndices.front() == completedRectangleRegion,
            "document should de-duplicate identical bounded-region selections");

#ifdef ELCAD_HAVE_OCCT
    ScopedOffscreenGlContext glContext;
    require(glContext.isValid(), "offscreen GL context should be available for renderer face-selection regression");

    Renderer renderer;
    auto bodyDoc = makeSingleBodyDocument(BRepPrimAPI_MakeBox(40.0, 30.0, 20.0).Shape(), "FaceSelectionBox");
    Body* body = bodyDoc->bodyByIndex(0);
    require(body && body->hasShape(), "single-body document should expose an OCCT body for renderer face selection");

    const auto selectedTriangles = renderer.resolveFaceSelectionTriangles(body, 0);
    require(selectedTriangles.size() >= 2, "resolved face selection should include the full tessellated face");

    const int faceOrd = renderer.faceOrdinalForTriangle(body, 0);
    require(faceOrd >= 0, "selected seed triangle should resolve to a face ordinal");
    for (int tri : selectedTriangles) {
        require(renderer.faceOrdinalForTriangle(body, tri) == faceOrd,
                "resolved face selection should stay within one OCCT face");
    }

    const TopoDS_Face directFace = faceByOrdinal(body->shape(), faceOrd);
    require(!directFace.IsNull(), "resolved face ordinal should map back to an OCCT face");

    const TopoDS_Face reconstructedFace = renderer.buildFaceFromTriangles(body, selectedTriangles);
    require(!reconstructedFace.IsNull(), "resolved face triangles should reconstruct a bounded OCCT face");
    require(BRepCheck_Analyzer(reconstructedFace).IsValid(),
            "reconstructed selected face should remain topologically valid");
    requireNear(static_cast<float>(faceArea(reconstructedFace)),
                static_cast<float>(faceArea(directFace)),
                0.5f,
                "renderer face selection should reconstruct the same bounded face area used by extrusion");
#endif
}

} // namespace elcad::test
