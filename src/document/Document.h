#pragma once
#include "document/Body.h"
#include <QObject>
#include <vector>
#include <memory>

namespace elcad {

class UndoStack;
class Sketch;
class SketchPlane;

class Document : public QObject {
    Q_OBJECT
public:
    explicit Document(QObject* parent = nullptr);
    ~Document() override;

    // Body management
    Body*  addBody(const QString& name = "Body");
    void   removeBody(quint64 id);
    // Remove body from document but return ownership (for undo)
    std::unique_ptr<Body> removeBodyRetain(quint64 id);
    // Re-insert a previously removed body (preserves original ID)
    void   reinsertBody(std::unique_ptr<Body> body);
    Body*  bodyById(quint64 id) const;
    Body*  bodyByIndex(int index) const;
    int    bodyCount() const;

    const std::vector<std::unique_ptr<Body>>& bodies() const { return m_bodies; }

    // Selection helpers
    struct SelectedItem {
        enum class Type { Body, Face, Edge, Vertex, SketchPoint, SketchLine, SketchArea };
        Type    type{Type::Body};
        quint64 bodyId{0};    // owning body (for Body/Face/Edge/Vertex types)
        quint64 sketchId{0};  // owning sketch (for Sketch* types)
        int     index{-1};    // face/edge/vertex index within body; area loop index for SketchArea
        quint64 entityId{0};  // sketch entity id (for SketchPoint and SketchLine)

        bool operator==(SelectedItem const& o) const noexcept {
            return type == o.type && bodyId == o.bodyId && sketchId == o.sketchId
                && index == o.index && entityId == o.entityId;
        }
    };

    void   clearSelection();
    void   addSelection(const SelectedItem& it);
    void   removeSelection(const SelectedItem& it);
    void   toggleSelection(const SelectedItem& it);
    bool   isSelected(const SelectedItem& it) const;
    std::vector<SelectedItem> selectionItems() const;

    Body*  singleSelectedBody() const;

    // Undo / redo
    UndoStack& undoStack() { return *m_undoStack; }

    // Sketch management
    Sketch* beginSketch(const SketchPlane& plane);
    Sketch* activeSketch() const { return m_activeSketch.get(); }
    void    endSketch();
    // Re-open a previously completed sketch for editing.
    void    reactivateSketch(Sketch* sketch);

    Sketch* sketchById(quint64 id) const;

    const std::vector<std::unique_ptr<Sketch>>& sketches() const { return m_sketches; }

signals:
    void bodyAdded(Body* body);
    void bodyRemoved(quint64 id);
    void bodyChanged(Body* body);   // geometry/mesh changed
    void selectionChanged();
    void activeSketchChanged(Sketch* sketch);  // nullptr when sketch ended

private:
    std::vector<std::unique_ptr<Body>>   m_bodies;
    std::unique_ptr<UndoStack>           m_undoStack;
    std::unique_ptr<Sketch>              m_activeSketch;
    std::vector<std::unique_ptr<Sketch>> m_sketches;

    // Current selection (mixed types allowed)
    std::vector<SelectedItem>            m_selection;
};

} // namespace elcad
