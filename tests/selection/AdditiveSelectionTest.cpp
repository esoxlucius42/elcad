#include "SelectionFixtures.h"

namespace elcad::test {

void runAdditiveSelectionTests()
{
    auto doc = makeBodyDocument(2);
    const quint64 firstBodyId = doc->bodyByIndex(0)->id();
    const quint64 secondBodyId = doc->bodyByIndex(1)->id();

    doc->setSelection({makeBodySelection(firstBodyId)});
    doc->addSelection(makeBodySelection(secondBodyId));

    require(doc->selectionItems().size() == 2,
            "plain additive selection should preserve the first body and add the second");
    require(doc->bodyById(firstBodyId)->selected(),
            "first body should remain highlighted after additive selection");
    require(doc->bodyById(secondBodyId)->selected(),
            "second body should become highlighted after additive selection");
    require(doc->singleSelectedBody() == nullptr,
            "singleSelectedBody should reject multi-body selections");

    const auto faceSelection = makeFaceSelection(firstBodyId, 4);
    const auto sketchSelection = makeSketchLineSelection(42, 7);
    doc->addSelections({faceSelection, sketchSelection, faceSelection});

    require(doc->selectionItems().size() == 4,
            "batch additive selection should de-duplicate repeated items while preserving mixed types");
    require(doc->isSelected(faceSelection),
            "face selection should be retained in the mixed selection set");
    require(doc->isSelected(sketchSelection),
            "sketch selection should be retained in the mixed selection set");

    doc->addSelection(makeBodySelection(secondBodyId));
    require(doc->selectionItems().size() == 4,
            "re-adding an already selected item should be a no-op for plain click semantics");

    doc->setSelection({faceSelection});
    require(!doc->bodyById(firstBodyId)->selected(),
            "face-only selection should not trigger whole-body highlight flags");
    require(doc->singleSelectedBody() == doc->bodyById(firstBodyId),
            "face-only selection should still resolve its owning body for single-target commands");
}

} // namespace elcad::test
