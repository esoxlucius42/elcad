#pragma once

#include "document/Document.h"
#include <memory>
#include <string>

namespace elcad::test {

std::unique_ptr<Document> makeBodyDocument(int bodyCount = 3);
Document::SelectedItem makeBodySelection(quint64 bodyId);
Document::SelectedItem makeFaceSelection(quint64 bodyId, int faceIndex);
Document::SelectedItem makeSketchLineSelection(quint64 sketchId, quint64 entityId);

void require(bool condition, const std::string& message);

void runAdditiveSelectionTests();
void runEmptySpaceClearTests();
void runShortcutClearTests();

} // namespace elcad::test
