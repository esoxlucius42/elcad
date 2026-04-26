#include "SketchIntersectionFixtures.h"

#include <cmath>
#include <set>

namespace elcad::test {

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
}

} // namespace elcad::test
