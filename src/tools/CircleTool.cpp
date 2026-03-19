#include "tools/CircleTool.h"
#include "sketch/Sketch.h"
#include <QKeyEvent>
#include <cmath>

namespace elcad {

CircleTool::CircleTool(Sketch* sketch, QObject* parent)
    : SketchTool(sketch, parent)
{}

void CircleTool::onMousePress(QVector2D pos, Qt::MouseButtons buttons,
                                Qt::KeyboardModifiers /*mods*/)
{
    if (buttons & Qt::RightButton) {
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
        return;
    }

    if (m_state == Idle) {
        m_center = pos;
        m_cursor = pos;
        m_state  = WaitingRadius;
    } else {
        float r = (pos - m_center).length();
        if (r > 0.5f)
            m_sketch->addCircle(m_center.x(), m_center.y(), r);
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
    }
}

void CircleTool::onMouseMove(QVector2D pos)
{
    m_cursor = pos;
    emit requestRedraw();
}

bool CircleTool::onKeyPress(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape && m_state != Idle) {
        // Cancel the in-progress circle; fall through on second Esc to exit sketch
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
        return true;
    }
    return false;
}

QString CircleTool::statusHint() const
{
    if (m_state == Idle)
        return "Circle: click center";
    float r = (m_cursor - m_center).length();
    return QString("Circle: click to set radius  |  r = %1 mm  |  RMB or Esc to cancel")
        .arg(r, 0, 'f', 2);
}

std::vector<SketchEntity> CircleTool::previewEntities() const
{
    if (m_state != WaitingRadius) return {};

    SketchEntity e(SketchEntity::Circle);
    e.p0     = m_center;
    e.radius = (m_cursor - m_center).length();
    return {e};
}

void CircleTool::reset()
{
    m_state = Idle;
    m_done  = false;
}

} // namespace elcad
