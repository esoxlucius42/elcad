#pragma once
#include "sketch/SketchEntity.h"
#include <QObject>
#include <QVector2D>
#include <vector>

QT_BEGIN_NAMESPACE
class QKeyEvent;
QT_END_NAMESPACE

namespace elcad {

class Sketch;

// Abstract base for all sketch drawing tools.
// Tools receive snapped 2D sketch-plane coordinates.
class SketchTool : public QObject {
    Q_OBJECT
public:
    explicit SketchTool(Sketch* sketch, QObject* parent = nullptr)
        : QObject(parent), m_sketch(sketch) {}

    virtual ~SketchTool() = default;

    // Mouse events in 2D sketch-plane coordinates (mm, snapped).
    virtual void onMousePress  (QVector2D pos, Qt::MouseButtons buttons,
                                Qt::KeyboardModifiers mods) = 0;
    virtual void onMouseMove   (QVector2D pos) = 0;
    virtual void onMouseRelease(QVector2D pos, Qt::MouseButtons buttons) {}

    // Returns false if the tool didn't handle the key (caller should handle).
    virtual bool onKeyPress(QKeyEvent* e) { Q_UNUSED(e) return false; }

    virtual QString statusHint() const = 0;

    // Preview entities to draw with the cursor (not yet committed to sketch).
    virtual std::vector<SketchEntity> previewEntities() const { return {}; }

    // True once the tool has finished its work (one entity placed).
    bool isDone() const { return m_done; }

    // Reset tool to initial state (for re-use after placing one entity).
    virtual void reset() { m_done = false; }

signals:
    void requestRedraw();

protected:
    Sketch* m_sketch{nullptr};
    bool    m_done{false};
};

} // namespace elcad
