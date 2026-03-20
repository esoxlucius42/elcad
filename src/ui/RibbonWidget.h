#pragma once
#include <QWidget>
#include <QToolButton>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QFrame;
QT_END_NAMESPACE

namespace elcad {

// A titled group of icon buttons inside the ribbon bar.
class RibbonGroup : public QWidget {
    Q_OBJECT
public:
    explicit RibbonGroup(const QString& title, QWidget* parent = nullptr);

    QToolButton* addButton(const QString& text, const QIcon& icon,
                           const QString& tooltip = {});
    void         addDivider();

private:
    QWidget*     m_btnArea{nullptr};
    QHBoxLayout* m_btnLayout{nullptr};
};

// Horizontal ribbon bar with Sketch / Transform / Modeling groups.
// The Drawing group is shown only while in sketch mode (call setSketchMode()).
class RibbonWidget : public QWidget {
    Q_OBJECT
public:
    explicit RibbonWidget(QWidget* parent = nullptr);

    // Toggle between 3D-mode groups and sketch Drawing group.
    void setSketchMode(bool active);

    // ── Sketch group ──────────────────────────────────────────────────────────
    QToolButton* btnFrontPlane{nullptr};
    QToolButton* btnTopPlane{nullptr};
    QToolButton* btnRightPlane{nullptr};

    // ── Transform group (checkable, radio-style) ──────────────────────────────
    QToolButton* btnMove{nullptr};
    QToolButton* btnRotate{nullptr};
    QToolButton* btnScale{nullptr};

    // ── Modeling group ────────────────────────────────────────────────────────
    QToolButton* btnExtrude{nullptr};
    QToolButton* btnMirror{nullptr};
    QToolButton* btnUnion{nullptr};
    QToolButton* btnSubtract{nullptr};

    // ── Drawing group (sketch mode only) ─────────────────────────────────────
    QToolButton* btnLine{nullptr};
    QToolButton* btnRect{nullptr};
    QToolButton* btnCircle{nullptr};
    QToolButton* btnConstr{nullptr};
    QToolButton* btnExitSketch{nullptr};

private:
    RibbonGroup* m_sketchGroup{nullptr};
    RibbonGroup* m_transformGroup{nullptr};
    RibbonGroup* m_modelingGroup{nullptr};
    RibbonGroup* m_drawingGroup{nullptr};
};

} // namespace elcad
