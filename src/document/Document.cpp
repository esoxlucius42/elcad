#include "document/Document.h"
#include "core/Logger.h"
#include "document/UndoStack.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPlane.h"
#include <algorithm>

namespace elcad {

Document::Document(QObject* parent)
    : QObject(parent)
    , m_undoStack(std::make_unique<UndoStack>())
{}

Document::~Document() = default;

Body* Document::addBody(const QString& name)
{
    auto body = std::make_unique<Body>(name);
    Body* ptr = body.get();
    m_bodies.push_back(std::move(body));
    LOG_INFO("Document: body added — id={} name='{}' total={}",
             ptr->id(), name.toStdString(), m_bodies.size());
    emit bodyAdded(ptr);
    return ptr;
}

void Document::removeBody(quint64 id)
{
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [id](const auto& b) { return b->id() == id; });
    if (it == m_bodies.end()) {
        LOG_WARN("Document::removeBody: body id={} not found — nothing removed", id);
        return;
    }
    LOG_INFO("Document: body removed — id={} name='{}' remaining={}",
             id, (*it)->name().toStdString(), m_bodies.size() - 1);
    m_bodies.erase(it);
    emit bodyRemoved(id);
}

std::unique_ptr<Body> Document::removeBodyRetain(quint64 id)
{
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [id](const auto& b) { return b->id() == id; });
    if (it == m_bodies.end()) {
        LOG_WARN("Document::removeBodyRetain: body id={} not found", id);
        return nullptr;
    }
    LOG_DEBUG("Document: body retained for undo — id={} name='{}'",
              id, (*it)->name().toStdString());
    auto body = std::move(*it);
    m_bodies.erase(it);
    emit bodyRemoved(id);
    return body;
}

void Document::reinsertBody(std::unique_ptr<Body> body)
{
    if (!body) return;
    LOG_DEBUG("Document: body reinserted (redo) — id={} name='{}'",
              body->id(), body->name().toStdString());
    Body* ptr = body.get();
    m_bodies.push_back(std::move(body));
    emit bodyAdded(ptr);
}

Body* Document::bodyById(quint64 id) const
{
    for (auto& b : m_bodies)
        if (b->id() == id) return b.get();
    return nullptr;
}

Body* Document::bodyByIndex(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_bodies.size())) return nullptr;
    return m_bodies[index].get();
}

int Document::bodyCount() const
{
    return static_cast<int>(m_bodies.size());
}

void Document::clearSelection()
{
    bool changed = false;
    for (auto& b : m_bodies) {
        if (b->selected()) { b->setSelected(false); changed = true; }
    }
    if (changed) emit selectionChanged();
}

Body* Document::singleSelectedBody() const
{
    Body* found = nullptr;
    for (auto& b : m_bodies) {
        if (b->selected()) {
            if (found) return nullptr;  // more than one
            found = b.get();
        }
    }
    return found;
}

Sketch* Document::beginSketch(const SketchPlane& plane)
{
    LOG_INFO("Document: sketch started on plane '{}'", plane.name().toStdString());
    m_activeSketch = std::make_unique<Sketch>(plane, this);
    emit activeSketchChanged(m_activeSketch.get());
    return m_activeSketch.get();
}

void Document::endSketch()
{
    if (!m_activeSketch) return;
    LOG_INFO("Document: sketch ended — {} entities retained",
             m_activeSketch->entities().size());
    m_sketches.push_back(std::move(m_activeSketch));
    emit activeSketchChanged(nullptr);
}

} // namespace elcad
