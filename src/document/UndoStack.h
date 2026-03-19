#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <QString>
#include <optional>

namespace elcad {

// Command interface for undo/redo
struct Command {
    virtual ~Command() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual QString description() const = 0;
};

// Convenience: lambda-based command
struct LambdaCommand : Command {
    QString              m_desc;
    std::function<void()> m_undo;
    std::function<void()> m_redo;

    LambdaCommand(QString desc,
                  std::function<void()> undoFn,
                  std::function<void()> redoFn)
        : m_desc(std::move(desc))
        , m_undo(std::move(undoFn))
        , m_redo(std::move(redoFn))
    {}

    void undo() override { m_undo(); }
    void redo() override { m_redo(); }
    QString description() const override { return m_desc; }
};

class UndoStack {
public:
    UndoStack() = default;

    // Push and immediately execute redo()
    void push(std::unique_ptr<Command> cmd);

    bool canUndo() const;
    bool canRedo() const;

    QString undoText() const;
    QString redoText() const;

    void undo();
    void redo();

    void clear();

    // Optional callback invoked after every push/undo/redo (e.g. to refresh UI)
    std::function<void()> onChange;

private:
    std::vector<std::unique_ptr<Command>> m_stack;
    int m_index{-1};  // points to the last executed command
};

} // namespace elcad
