#include "SelectionFixtures.h"

#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"

namespace elcad::test {

void runEmptySpaceClearTests()
{
    auto doc = makeBodyDocument(2);
    const quint64 firstBodyId = doc->bodyByIndex(0)->id();
    const quint64 secondBodyId = doc->bodyByIndex(1)->id();

    auto* completedSketch = doc->beginSketch(SketchPlane::xy());
    auto* completedLine = completedSketch->addLine(0.f, 0.f, 10.f, 0.f);
    completedLine->selected = true;
    doc->endSketch();

    auto* activeSketch = doc->beginSketch(SketchPlane::xy());
    auto* activeLine = activeSketch->addLine(0.f, 0.f, 0.f, 10.f);
    activeLine->selected = true;

    doc->setSelection({
        makeBodySelection(firstBodyId),
        makeBodySelection(secondBodyId),
        makeSketchLineSelection(doc->sketches().front()->id(), completedLine->id)
    });

    doc->clearSelection();

    require(doc->selectionItems().empty(),
            "empty-space clear should remove every selected item from the selection set");
    require(!doc->bodyById(firstBodyId)->selected() && !doc->bodyById(secondBodyId)->selected(),
            "empty-space clear should reset whole-body highlight flags");
    require(!doc->sketches().front()->entities().front()->selected,
            "empty-space clear should clear completed-sketch entity selection state");
    require(!activeLine->selected,
            "empty-space clear should clear active-sketch entity selection state");
}

} // namespace elcad::test
