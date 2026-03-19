#include "tools/RectTool.h"
#include "sketch/Sketch.h"
#include <QKeyEvent>

namespace elcad {

RectTool::RectTool(Sketch* sketch, QObject* parent)
    : SketchTool(sketch, parent)
{}

void RectTool::onMousePress(QVector2D pos, Qt::MouseButtons buttons,
                              Qt::KeyboardModifiers /*mods*/)
{
    if (buttons & Qt::RightButton) {
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
        return;
    }

    if (m_state == Idle) {
        m_corner1 = pos;
        m_cursor  = pos;
        m_state   = WaitingCorner2;
    } else {
        // Commit 4 lines
        float x0 = m_corner1.x(), y0 = m_corner1.y();
        float x1 = pos.x(),       y1 = pos.y();
        m_sketch->addLine(x0, y0, x1, y0);  // bottom
        m_sketch->addLine(x1, y0, x1, y1);  // right
        m_sketch->addLine(x1, y1, x0, y1);  // top
        m_sketch->addLine(x0, y1, x0, y0);  // left
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
    }
}

void RectTool::onMouseMove(QVector2D pos)
{
    m_cursor = pos;
    emit requestRedraw();
}

bool RectTool::onKeyPress(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape && m_state != Idle) {
        // Cancel the in-progress rectangle; fall through on second Esc to exit sketch
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
        return true;
    }
    return false;
}

QString RectTool::statusHint() const
{
    if (m_state == Idle)            return "Rectangle: click first corner";
    if (m_state == WaitingCorner2)  return "Rectangle: click opposite corner  |  RMB or Esc to cancel";
    return {};
}

std::vector<SketchEntity> RectTool::previewEntities() const
{
    if (m_state != WaitingCorner2) return {};

    float x0 = m_corner1.x(), y0 = m_corner1.y();
    float x1 = m_cursor.x(),  y1 = m_cursor.y();

    auto makeLine = [](float ax, float ay, float bx, float by) {
        SketchEntity e(SketchEntity::Line);
        e.p0 = {ax, ay};
        e.p1 = {bx, by};
        return e;
    };

    return {
        makeLine(x0, y0, x1, y0),
        makeLine(x1, y0, x1, y1),
        makeLine(x1, y1, x0, y1),
        makeLine(x0, y1, x0, y0),
    };
}

void RectTool::reset()
{
    m_state = Idle;
    m_done  = false;
}

} // namespace elcad
