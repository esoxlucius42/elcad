#pragma once
#include "tools/SketchTool.h"
#include <QVector2D>

namespace elcad {

// CircleTool: click center, click any point on circumference → commits circle.
class CircleTool : public SketchTool {
    Q_OBJECT
public:
    explicit CircleTool(Sketch* sketch, QObject* parent = nullptr);

    void onMousePress  (QVector2D pos, Qt::MouseButtons, Qt::KeyboardModifiers) override;
    void onMouseMove   (QVector2D pos) override;
    bool onKeyPress    (QKeyEvent* e)  override;

    QString statusHint() const override;
    std::vector<SketchEntity> previewEntities() const override;

    void reset() override;

private:
    enum State { Idle, WaitingRadius };
    State     m_state{Idle};
    QVector2D m_center;
    QVector2D m_cursor;
};

} // namespace elcad
