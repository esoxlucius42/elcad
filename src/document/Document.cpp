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

namespace {

bool isSketchSelectionType(Document::SelectedItem::Type type)
{
    return type == Document::SelectedItem::Type::SketchPoint
        || type == Document::SelectedItem::Type::SketchLine
        || type == Document::SelectedItem::Type::SketchArea;
}

bool sketchHasSelectedEntities(const Sketch* sketch)
{
    if (!sketch)
        return false;

    return std::any_of(sketch->entities().begin(),
                       sketch->entities().end(),
                       [](const auto& entity) {
                           return entity && entity->selected;
                       });
}

std::vector<Document::SelectedItem> dedupeSelectionItems(const std::vector<Document::SelectedItem>& items)
{
    std::vector<Document::SelectedItem> deduped;
    deduped.reserve(items.size());

    for (const auto& item : items) {
        if (std::find(deduped.begin(), deduped.end(), item) == deduped.end())
            deduped.push_back(item);
    }

    return deduped;
}

} // namespace

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
    bool changed = !m_selection.empty();
    m_selection.clear();

    if (m_activeSketch && sketchHasSelectedEntities(m_activeSketch.get())) {
        m_activeSketch->clearSelection();
        changed = true;
    }

    for (const auto& sketch : m_sketches) {
        if (sketchHasSelectedEntities(sketch.get())) {
            sketch->clearSelection();
            changed = true;
        }
    }

    if (syncBodySelectionFlags())
        changed = true;

    if (changed)
        emit selectionChanged();
}

void Document::setSelection(const std::vector<SelectedItem>& items)
{
    auto nextSelection = dedupeSelectionItems(items);
    bool changed = nextSelection != m_selection;
    m_selection = std::move(nextSelection);

    if (syncBodySelectionFlags())
        changed = true;

    if (changed)
        emit selectionChanged();
}

void Document::addSelection(const SelectedItem& it)
{
    addSelections({it});
}

void Document::addSelections(const std::vector<SelectedItem>& items)
{
    bool changed = false;
    for (const auto& item : dedupeSelectionItems(items)) {
        if (!isSelected(item)) {
            m_selection.push_back(item);
            changed = true;
        }
    }

    if (syncBodySelectionFlags())
        changed = true;

    if (changed)
        emit selectionChanged();
}

void Document::removeSelection(const SelectedItem& it)
{
    auto itPos = std::find_if(m_selection.begin(), m_selection.end(),
        [&](const SelectedItem& s){ return s == it; });
    if (itPos == m_selection.end())
        return;

    m_selection.erase(itPos);
    bool changed = true;

    if (syncBodySelectionFlags())
        changed = true;

    if (changed)
        emit selectionChanged();
}

void Document::toggleSelection(const SelectedItem& it)
{
    toggleSelections({it});
}

void Document::toggleSelections(const std::vector<SelectedItem>& items)
{
    bool changed = false;
    for (const auto& item : dedupeSelectionItems(items)) {
        auto itPos = std::find_if(m_selection.begin(), m_selection.end(),
            [&](const SelectedItem& selected) { return selected == item; });
        if (itPos == m_selection.end()) {
            m_selection.push_back(item);
            changed = true;
        } else {
            m_selection.erase(itPos);
            changed = true;
        }
    }

    if (syncBodySelectionFlags())
        changed = true;

    if (changed)
        emit selectionChanged();
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

bool Document::syncBodySelectionFlags()
{
    bool changed = false;
    for (auto& body : m_bodies) {
        const bool shouldSelect = std::any_of(m_selection.begin(),
                                              m_selection.end(),
                                              [&](const SelectedItem& item) {
                                                  return !isSketchSelectionType(item.type)
                                                      && item.bodyId == body->id();
                                              });
        if (body->selected() != shouldSelect) {
            body->setSelected(shouldSelect);
            changed = true;
        }
    }
    return changed;
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

void Document::clearSketchSelection(quint64 sketchId)
{
    if (sketchId == 0)
        return;

    if (m_activeSketch && m_activeSketch->id() == sketchId)
        m_activeSketch->clearSelection();
    if (Sketch* sketch = sketchById(sketchId))
        sketch->clearSelection();

    auto it = std::remove_if(m_selection.begin(),
                             m_selection.end(),
                             [sketchId](const SelectedItem& item) {
                                 return item.sketchId == sketchId;
                             });
    if (it == m_selection.end())
        return;

    m_selection.erase(it, m_selection.end());
    bool changed = true;
    if (syncBodySelectionFlags())
        changed = true;
    if (changed)
        emit selectionChanged();
}

Body* Document::singleSelectedBody() const
{
    Body* found = nullptr;
    for (auto& body : m_bodies) {
        const bool bodySelected = std::any_of(m_selection.begin(),
                                              m_selection.end(),
                                              [&](const SelectedItem& item) {
                                                  return !isSketchSelectionType(item.type)
                                                      && item.bodyId == body->id();
                                              });
        if (!bodySelected)
            continue;

        if (found)
            return nullptr;

        found = body.get();
    }

    if (found)
        return found;

    for (auto& body : m_bodies) {
        if (body->selected()) {
            if (found)
                return nullptr;
            found = body.get();
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
    clearSketchSelection((*it)->id());
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
        if (!visible)
            clearSketchSelection(id);
        emit sketchVisibilityChanged(s);
    }
}

} // namespace elcad
