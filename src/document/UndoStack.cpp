#include "document/UndoStack.h"
#include "core/Logger.h"
#include <stdexcept>

namespace elcad {

void UndoStack::push(std::unique_ptr<Command> cmd)
{
    // Discard any redoable commands above current position
    int discarded = static_cast<int>(m_stack.size()) - (m_index + 1);
    if (discarded > 0) {
        LOG_DEBUG("UndoStack: discarding {} redoable command(s) above index {}",
                  discarded, m_index);
        m_stack.erase(m_stack.begin() + m_index + 1, m_stack.end());
    }

    LOG_DEBUG("UndoStack: pushing '{}' — stack size will be {}",
              cmd->description().toStdString(), m_stack.size() + 1);
    cmd->redo();  // execute immediately
    m_stack.push_back(std::move(cmd));
    m_index = static_cast<int>(m_stack.size()) - 1;
    if (onChange) onChange();
}

bool UndoStack::canUndo() const { return m_index >= 0; }
bool UndoStack::canRedo() const { return m_index + 1 < static_cast<int>(m_stack.size()); }

QString UndoStack::undoText() const
{
    return canUndo() ? m_stack[m_index]->description() : QString();
}

QString UndoStack::redoText() const
{
    return canRedo() ? m_stack[m_index + 1]->description() : QString();
}

void UndoStack::undo()
{
    if (!canUndo()) {
        LOG_DEBUG("UndoStack::undo called but stack is empty or at bottom");
        return;
    }
    LOG_INFO("Undo: '{}' (index {} → {})",
             m_stack[m_index]->description().toStdString(), m_index, m_index - 1);
    m_stack[m_index]->undo();
    --m_index;
    if (onChange) onChange();
}

void UndoStack::redo()
{
    if (!canRedo()) {
        LOG_DEBUG("UndoStack::redo called but nothing to redo");
        return;
    }
    ++m_index;
    LOG_INFO("Redo: '{}' (index {} → {})",
             m_stack[m_index]->description().toStdString(), m_index - 1, m_index);
    m_stack[m_index]->redo();
    if (onChange) onChange();
}

void UndoStack::clear()
{
    LOG_DEBUG("UndoStack: cleared ({} command(s) discarded)", m_stack.size());
    m_stack.clear();
    m_index = -1;
}

} // namespace elcad
