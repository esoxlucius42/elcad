#include "tools/LineTool.h"
#include "sketch/Sketch.h"
#include <QKeyEvent>

namespace elcad {

LineTool::LineTool(Sketch* sketch, bool construction, QObject* parent)
    : SketchTool(sketch, parent)
    , m_construction(construction)
{}

void LineTool::onMousePress(QVector2D pos, Qt::MouseButtons buttons,
                             Qt::KeyboardModifiers /*mods*/)
{
    if (buttons & Qt::RightButton) {
        // Right-click finishes the chain
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
        return;
    }

    if (m_state == Idle) {
        m_p0     = pos;
        m_cursor = pos;
        m_state  = WaitingP1;
    } else {
        // Commit line
        if ((pos - m_p0).lengthSquared() > 0.01f) {
            m_sketch->addLine(m_p0.x(), m_p0.y(), pos.x(), pos.y(), m_construction);
        }
        // Chain: new start = previous end
        m_p0    = pos;
        m_state = WaitingP1;
    }
    emit requestRedraw();
}

void LineTool::onMouseMove(QVector2D pos)
{
    m_cursor = pos;
    emit requestRedraw();
}

bool LineTool::onKeyPress(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape && m_state != Idle) {
        // Cancel the in-progress line segment; fall through on second Esc to exit sketch
        m_state = Idle;
        m_done  = true;
        emit requestRedraw();
        return true;
    }
    return false;
}

QString LineTool::statusHint() const
{
    if (m_state == Idle)       return m_construction ? "Construction Line: click to start" : "Line: click to start";
    if (m_state == WaitingP1)  return "Line: click end point  |  RMB or Esc to finish";
    return {};
}

std::vector<SketchEntity> LineTool::previewEntities() const
{
    if (m_state != WaitingP1) return {};

    SketchEntity e(SketchEntity::Line);
    e.p0           = m_p0;
    e.p1           = m_cursor;
    e.construction = m_construction;
    return {e};
}

void LineTool::reset()
{
    m_state = Idle;
    m_done  = false;
}

} // namespace elcad
