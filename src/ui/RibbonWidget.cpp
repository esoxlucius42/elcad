#include "ui/RibbonWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QToolButton>

namespace elcad {

// ── Shared style strings ──────────────────────────────────────────────────────

static const char* kGroupQSS = R"(
RibbonGroup {
    background: #1a1a1a;
    border-right: 1px solid #383838;
}
)";

static const char* kBtnQSS = R"(
QToolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 4px 6px 2px 6px;
    color: #c8c8c8;
    font-size: 9px;
}
QToolButton:hover {
    background: #3a3a3a;
    border-color: #555555;
}
QToolButton:checked {
    background: #1b3a6b;
    border-color: #2979ff;
    color: #90caff;
}
QToolButton:pressed {
    background: #243f78;
    border-color: #2979ff;
}
QToolButton:disabled {
    color: #555555;
}
)";

static const char* kSmallBtnQSS = R"(
QToolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 3px;
    padding: 3px 6px;
    color: #c8c8c8;
    font-size: 10px;
    min-width: 44px;
    min-height: 26px;
}
QToolButton:hover {
    background: #3a3a3a;
    border-color: #555555;
}
QToolButton:checked {
    background: #1b3a6b;
    border-color: #2979ff;
    color: #90caff;
}
QToolButton:pressed {
    background: #243f78;
    border-color: #2979ff;
}
)";

// ── RibbonGroup ───────────────────────────────────────────────────────────────

RibbonGroup::RibbonGroup(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet(kGroupQSS);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(6, 5, 6, 4);
    outer->setSpacing(2);

    m_btnArea = new QWidget(this);
    m_btnArea->setStyleSheet("background: transparent;");
    m_btnLayout = new QHBoxLayout(m_btnArea);
    m_btnLayout->setContentsMargins(0, 0, 0, 0);
    m_btnLayout->setSpacing(3);

    auto* lbl = new QLabel(title, this);
    lbl->setAlignment(Qt::AlignHCenter);
    lbl->setStyleSheet("color: #686868; font-size: 9px; background: transparent;");

    outer->addWidget(m_btnArea, 1);
    outer->addWidget(lbl, 0);
}

QToolButton* RibbonGroup::addButton(const QString& text, const QIcon& icon,
                                    const QString& tooltip)
{
    auto* btn = new QToolButton(m_btnArea);

    if (icon.isNull()) {
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setStyleSheet(kSmallBtnQSS);
    } else {
        btn->setIcon(icon);
        btn->setIconSize(QSize(48, 48));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setStyleSheet(kBtnQSS);
        btn->setFixedSize(64, 70);
    }

    btn->setText(text);
    if (!tooltip.isEmpty())
        btn->setToolTip(tooltip);

    m_btnLayout->addWidget(btn);
    return btn;
}

void RibbonGroup::addDivider()
{
    auto* sep = new QFrame(m_btnArea);
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedWidth(1);
    sep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    sep->setStyleSheet("background: #3a3a3a; border: none;");
    m_btnLayout->addSpacing(4);
    m_btnLayout->addWidget(sep);
    m_btnLayout->addSpacing(4);
}

// ── RibbonWidget ──────────────────────────────────────────────────────────────

RibbonWidget::RibbonWidget(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("RibbonWidget { background: #1a1a1a; border-bottom: 1px solid #333333; }");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(98);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(0);

    // ── Sketch group ──────────────────────────────────────────────────────────
    m_sketchGroup = new RibbonGroup("Sketch", this);

    btnFrontPlane = m_sketchGroup->addButton(
        "Front\nPlane", QIcon(":/assets/icons/front-plane.svg"),
        "Front Plane\nEnter sketch on XZ plane");

    btnTopPlane = m_sketchGroup->addButton(
        "Top\nPlane", QIcon(":/assets/icons/top-plane.svg"),
        "Top Plane\nEnter sketch on XY plane");

    btnRightPlane = m_sketchGroup->addButton(
        "Right\nPlane", QIcon(":/assets/icons/right-plane.svg"),
        "Right Plane\nEnter sketch on YZ plane");

    // ── Transform group ───────────────────────────────────────────────────────
    m_transformGroup = new RibbonGroup("Transform", this);

    btnMove = m_transformGroup->addButton(
        "Move", QIcon(":/assets/icons/move.svg"),
        "Move\nShortcut: T");
    btnMove->setCheckable(true);
    btnMove->setChecked(true);

    btnRotate = m_transformGroup->addButton(
        "Rotate", QIcon(":/assets/icons/rotate.svg"),
        "Rotate\nShortcut: R");
    btnRotate->setCheckable(true);

    btnScale = m_transformGroup->addButton(
        "Scale", QIcon(":/assets/icons/scale.svg"),
        "Scale\nShortcut: S");
    btnScale->setCheckable(true);

    // ── Modeling group ────────────────────────────────────────────────────────
    m_modelingGroup = new RibbonGroup("Modeling", this);

    btnExtrude = m_modelingGroup->addButton(
        "Extrude", QIcon(":/assets/icons/extrude.svg"), "Extrude");

    btnMirror = m_modelingGroup->addButton(
        "Mirror", QIcon(":/assets/icons/mirror.svg"), "Mirror");

    m_modelingGroup->addDivider();

    btnUnion = m_modelingGroup->addButton(
        "Union", QIcon(":/assets/icons/union.svg"), "Union");

    btnSubtract = m_modelingGroup->addButton(
        "Subtract", QIcon(":/assets/icons/subtract.svg"), "Subtract");

    // ── Drawing group (sketch mode, initially hidden) ─────────────────────────
    m_drawingGroup = new RibbonGroup("Drawing Tools", this);
    m_drawingGroup->setVisible(false);

    btnLine       = m_drawingGroup->addButton("Line",    {}, "Line tool (chained)");
    btnLine->setCheckable(true);

    btnRect       = m_drawingGroup->addButton("Rect",    {}, "Rectangle tool");
    btnRect->setCheckable(true);

    btnCircle     = m_drawingGroup->addButton("Circle",  {}, "Circle tool");
    btnCircle->setCheckable(true);

    btnConstr     = m_drawingGroup->addButton("Constr.", {}, "Construction line tool");
    btnConstr->setCheckable(true);

    btnExitSketch = m_drawingGroup->addButton("⏎ Done",  {}, "Exit Sketch — return to 3D mode");

    layout->addWidget(m_sketchGroup);
    layout->addWidget(m_transformGroup);
    layout->addWidget(m_modelingGroup);
    layout->addWidget(m_drawingGroup);
    layout->addStretch(1);
}

void RibbonWidget::setSketchMode(bool active)
{
    m_transformGroup->setVisible(!active);
    m_modelingGroup->setVisible(!active);
    m_drawingGroup->setVisible(active);
}

} // namespace elcad
