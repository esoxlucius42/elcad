#include "document/Document.h"
#include "core/Logger.h"
#include "document/UndoStack.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPicker.h"
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
    if (!m_selection.empty()) {
        m_selection.clear();
        changed = true;
    }

    // Keep backwards compatibility: clear body->selected flags
    for (auto& b : m_bodies) {
        if (b->selected()) { b->setSelected(false); changed = true; }
    }

    if (changed) emit selectionChanged();
}

void Document::addSelection(const SelectedItem& it)
{
    if (isSelected(it)) return;
    m_selection.push_back(it);

    // Only update body selection flags for body-related item types.
    bool isSketchType = (it.type == SelectedItem::Type::SketchPoint ||
                         it.type == SelectedItem::Type::SketchLine  ||
                         it.type == SelectedItem::Type::SketchArea);
    if (!isSketchType) {
        if (Body* b = bodyById(it.bodyId)) b->setSelected(true);
    }

    emit selectionChanged();
}

void Document::removeSelection(const SelectedItem& it)
{
    auto itPos = std::find_if(m_selection.begin(), m_selection.end(),
        [&](const SelectedItem& s){ return s == it; });
    if (itPos == m_selection.end()) return;
    m_selection.erase(itPos);

    bool isSketchType = (it.type == SelectedItem::Type::SketchPoint ||
                         it.type == SelectedItem::Type::SketchLine  ||
                         it.type == SelectedItem::Type::SketchArea);
    if (!isSketchType) {
        if (it.type == SelectedItem::Type::Body) {
            if (Body* b = bodyById(it.bodyId)) b->setSelected(false);
        } else {
            // For sub-item (face/edge/vertex) removal: deselect the body only when it has
            // no remaining selection items at all.
            if (Body* b = bodyById(it.bodyId)) {
                bool stillSelected = std::any_of(m_selection.begin(), m_selection.end(),
                    [&](const SelectedItem& s){ return s.bodyId == it.bodyId; });
                if (!stillSelected) b->setSelected(false);
            }
        }
    }

    emit selectionChanged();
}

void Document::toggleSelection(const SelectedItem& it)
{
    if (isSelected(it)) removeSelection(it);
    else addSelection(it);
}

bool Document::isSelected(const SelectedItem& it) const
{
    return std::any_of(m_selection.begin(), m_selection.end(),
                       [&](const SelectedItem& s){ return s == it; });
}

std::vector<Document::SelectedItem> Document::selectionItems() const
{
    return m_selection;
}

std::optional<SketchFaceSelection> Document::selectedSketchFaces(quint64 sketchId) const
{
    SketchFaceSelection selection;
    selection.sketchId = sketchId;

    for (const auto& item : m_selection) {
        if (item.type != SelectedItem::Type::SketchArea) continue;
        if (item.index < 0) continue;

        if (selection.sketchId == 0)
            selection.sketchId = item.sketchId;

        if (item.sketchId != selection.sketchId) {
            LOG_WARN("Document::selectedSketchFaces: mixed sketch-area selection is unsupported "
                     "(expected sketchId={}, saw sketchId={})",
                     selection.sketchId, item.sketchId);
            return std::nullopt;
        }

        if (std::find(selection.loopIndices.begin(),
                      selection.loopIndices.end(),
                      item.index) == selection.loopIndices.end()) {
            selection.loopIndices.push_back(item.index);
        }
    }

    if (selection.sketchId == 0 || selection.loopIndices.empty())
        return std::nullopt;

    return selection;
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
    m_activeSketch->setName(QString::asprintf("Sketch%02d", m_nextSketchNumber++));
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
    emit sketchAdded(m_sketches.back().get());
}

void Document::reactivateSketch(Sketch* sketch)
{
    if (!sketch) return;
    auto it = std::find_if(m_sketches.begin(), m_sketches.end(),
        [sketch](const auto& s) { return s.get() == sketch; });
    if (it == m_sketches.end()) {
        LOG_WARN("Document::reactivateSketch: sketch not found in completed list");
        return;
    }
    // Push any currently active sketch back to the completed list.
    if (m_activeSketch) {
        m_sketches.push_back(std::move(m_activeSketch));
        emit sketchAdded(m_sketches.back().get());
    }

    emit sketchRemoved((*it)->id());
    m_activeSketch = std::move(*it);
    m_sketches.erase(it);

    LOG_INFO("Document: sketch reactivated — id={} {} entities",
             m_activeSketch->id(), m_activeSketch->entities().size());
    emit activeSketchChanged(m_activeSketch.get());
}

Sketch* Document::sketchById(quint64 id) const
{
    for (auto& s : m_sketches)
        if (s->id() == id) return s.get();
    return nullptr;
}

void Document::setSketchVisible(quint64 id, bool visible)
{
    if (Sketch* s = sketchById(id)) {
        s->setVisible(visible);
        if (!visible) {
            s->clearSelection();
            auto it = std::remove_if(m_selection.begin(), m_selection.end(),
                [id](const SelectedItem& si) { return si.sketchId == id; });
            if (it != m_selection.end()) {
                m_selection.erase(it, m_selection.end());
                emit selectionChanged();
            }
        }
        emit sketchVisibilityChanged(s);
    }
}

} // namespace elcad
