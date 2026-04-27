#include "SelectionFixtures.h"

#include "sketch/SketchPlane.h"
#include <stdexcept>

namespace elcad::test {

std::unique_ptr<Document> makeBodyDocument(int bodyCount)
{
    auto doc = std::make_unique<Document>();
    for (int i = 0; i < bodyCount; ++i)
        doc->addBody(QString("Body_%1").arg(i + 1));
    return doc;
}

Document::SelectedItem makeBodySelection(quint64 bodyId)
{
    Document::SelectedItem item;
    item.type = Document::SelectedItem::Type::Body;
    item.bodyId = bodyId;
    item.index = -1;
    return item;
}

Document::SelectedItem makeFaceSelection(quint64 bodyId, int faceIndex)
{
    Document::SelectedItem item;
    item.type = Document::SelectedItem::Type::Face;
    item.bodyId = bodyId;
    item.index = faceIndex;
    return item;
}

Document::SelectedItem makeSketchLineSelection(quint64 sketchId, quint64 entityId)
{
    Document::SelectedItem item;
    item.type = Document::SelectedItem::Type::SketchLine;
    item.sketchId = sketchId;
    item.entityId = entityId;
    item.index = -1;
    return item;
}

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

} // namespace elcad::test
