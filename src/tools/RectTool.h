#pragma once
#include "tools/SketchTool.h"
#include <QVector2D>

namespace elcad {

// RectTool: click corner 1, click corner 2 → commits 4 lines.
class RectTool : public SketchTool {
    Q_OBJECT
public:
    explicit RectTool(Sketch* sketch, QObject* parent = nullptr);

    void onMousePress  (QVector2D pos, Qt::MouseButtons, Qt::KeyboardModifiers) override;
    void onMouseMove   (QVector2D pos) override;
    bool onKeyPress    (QKeyEvent* e)  override;

    QString statusHint() const override;
    std::vector<SketchEntity> previewEntities() const override;

    bool isInProgress() const override { return m_state == WaitingCorner2; }

    void reset() override;

private:
    enum State { Idle, WaitingCorner2 };
    State     m_state{Idle};
    QVector2D m_corner1;
    QVector2D m_cursor;
};

} // namespace elcad
