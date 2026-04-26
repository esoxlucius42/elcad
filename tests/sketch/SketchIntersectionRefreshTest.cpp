#include "SketchIntersectionFixtures.h"

namespace elcad::test {

void runSketchIntersectionRefreshTests()
{
    auto doc = makeCompletedOverlapDocument();
    require(doc->sketches().size() == 1, "completed overlap document should start with one sketch");

    Sketch* completedSketch = doc->sketches().front().get();
    const auto initialTopology = deriveBoundedRegions(*completedSketch);
    const int outerCircleRegion = findSelectableRegionIndex(initialTopology, QVector2D(-5.0f, 5.0f));
    require(outerCircleRegion >= 0,
            "outer circle region should be selectable before editing; " + describeBoundedRegions(initialTopology));

    Document::SelectedItem selectedArea;
    selectedArea.type = Document::SelectedItem::Type::SketchArea;
    selectedArea.sketchId = completedSketch->id();
    selectedArea.index = outerCircleRegion;
    doc->addSelection(selectedArea);
    require(doc->selectedSketchFaces(completedSketch->id()).has_value(),
            "document should expose the selected bounded region before editing");

    doc->reactivateSketch(completedSketch);
    require(doc->activeSketch() != nullptr, "reactivated sketch should become the active sketch");
    require(!doc->selectedSketchFaces(completedSketch->id()).has_value(),
            "reactivating a sketch should invalidate stale bounded-region selections");

    Sketch* activeSketch = doc->activeSketch();
    quint64 circleId = 0;
    for (const auto& entity : activeSketch->entities()) {
        if (entity->type == SketchEntity::Circle) {
            circleId = entity->id;
            break;
        }
    }
    require(circleId != 0, "reactivated overlap sketch should still contain the circle entity");

    activeSketch->removeEntity(circleId);
    activeSketch->addCircle(30.0f, 10.0f, 10.0f);
    doc->endSketch();

    require(doc->sketches().size() == 1, "edited sketch should return to the completed sketch list");
    Sketch* updatedSketch = doc->sketches().front().get();
    const auto tangentTopology = deriveBoundedRegions(*updatedSketch);
    require(tangentTopology.size() == 2,
            "tangent-only rectangle-circle contact should not create an extra overlap region");
    require(findSelectableRegionIndex(tangentTopology, QVector2D(10.0f, 10.0f)) >= 0,
            "rectangle region should remain selectable after the edit");
    require(findSelectableRegionIndex(tangentTopology, QVector2D(30.0f, 10.0f)) >= 0,
            "tangent circle region should remain selectable after the edit");

    auto openSketch = makeOpenRectangleSketch();
    require(deriveBoundedRegions(*openSketch).empty(),
            "open sketch geometry should not expose a bounded selectable region");

    auto duplicateSketch = makeRectangleWithDuplicateEdgeSketch();
    const auto duplicateTopology = deriveBoundedRegions(*duplicateSketch);
    require(duplicateTopology.size() == 1,
            "duplicate coincident boundaries should not create phantom bounded regions");
}

} // namespace elcad::test
