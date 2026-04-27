#include "SelectionFixtures.h"

#include "ui/ViewportWidget.h"
#include <QKeyEvent>

namespace {

class TestViewportWidget : public elcad::ViewportWidget {
public:
    using elcad::ViewportWidget::keyPressEvent;
};

} // namespace

namespace elcad::test {

void runShortcutClearTests()
{
    auto doc = makeBodyDocument(1);
    const quint64 bodyId = doc->bodyByIndex(0)->id();
    doc->setSelection({makeBodySelection(bodyId), makeSketchLineSelection(99, 11)});

    TestViewportWidget viewport;
    viewport.setDocument(doc.get());

    QKeyEvent spacePress(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    viewport.keyPressEvent(&spacePress);

    require(doc->selectionItems().empty(),
            "Space should clear the full selection set");
    require(!doc->bodyById(bodyId)->selected(),
            "Space should reset whole-body selection flags");

    viewport.keyPressEvent(&spacePress);
    require(doc->selectionItems().empty(),
            "Space on an already empty selection should remain a no-op");
}

} // namespace elcad::test
