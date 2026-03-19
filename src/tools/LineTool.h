#pragma once
#include "tools/SketchTool.h"
#include <QVector2D>

namespace elcad {

// LineTool: chained two-click line drawing.
// Click 1: set start point.
// Click 2: commit line, start of next segment = end of last.
// Right-click or Escape: finish chain.
class LineTool : public SketchTool {
    Q_OBJECT
public:
    explicit LineTool(Sketch* sketch, bool construction = false,
                      QObject* parent = nullptr);

    void onMousePress  (QVector2D pos, Qt::MouseButtons, Qt::KeyboardModifiers) override;
    void onMouseMove   (QVector2D pos) override;
    void onMouseRelease(QVector2D pos, Qt::MouseButtons) override {}
    bool onKeyPress    (QKeyEvent* e)  override;

    QString statusHint() const override;
    std::vector<SketchEntity> previewEntities() const override;

    void reset() override;

private:
    enum State { Idle, WaitingP1 };
    State     m_state{Idle};
    QVector2D m_p0;
    QVector2D m_cursor;
    bool      m_construction{false};
};

} // namespace elcad
