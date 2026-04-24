#pragma once
#include <QWidget>
#include "ui/ExtrudeDialog.h"

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace elcad {

class Document;

// Inline panel shown in the right sidebar below the NavCube gizmo.
// Displays controls for the active 3D tool, or an idle placeholder when
// no tool is pending. Replaces the floating tool dialogs.
class ToolOptionsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ToolOptionsPanel(QWidget* parent = nullptr);

    void setDocument(Document* doc);

    // Switch the displayed page.
    void showIdle();
    void showExtrude();
    void showMirror();
    void showBooleanUnion();
    void showBooleanSubtract();

    // Set the extrude distance spinbox value (triggers live-preview chain).
    void setExtrudeDistance(double dist);

    // Set the extrude distance without emitting extrudeParamsChanged.
    // Use this when the caller will trigger the preview directly.
    void setExtrudeDistanceSilent(double dist);

    // Return current extrude params from the UI controls.
    ExtrudeParams currentExtrudeParams() const;

signals:
    void extrudeRequested(ExtrudeParams params);
    void mirrorRequested(int mirrorPlane);
    void booleanUnionRequested(quint64 targetId, quint64 toolId);
    void booleanSubtractRequested(quint64 targetId, quint64 toolId);
    void cancelled();

    // Live-preview signals — emitted as parameters change (before Apply is clicked)
    void extrudeParamsChanged(ExtrudeParams params);
    void mirrorPlaneChanged(int plane);

private:
    void buildIdlePage();
    void buildExtrudePage();
    void buildMirrorPage();
    void buildBooleanPage(); // shared layout; label text set per operation

    void populateBooleanCombos();

    Document*       m_doc{nullptr};
    QStackedWidget* m_stack{nullptr};

    // ── Extrude page ──────────────────────────────────────────────────────────
    QDoubleSpinBox* m_extrudeDist{nullptr};
    QComboBox*      m_extrudeMode{nullptr};
    QCheckBox*      m_extrudeSym{nullptr};

    // ── Mirror page ───────────────────────────────────────────────────────────
    QComboBox*      m_mirrorPlane{nullptr};

    // ── Boolean page (shared; hidden label text distinguishes union/subtract) ─
    QComboBox*      m_boolTarget{nullptr};
    QComboBox*      m_boolTool{nullptr};
    QPushButton*    m_boolApply{nullptr};
    bool            m_boolIsSubtract{false};

    enum Page { Idle = 0, Extrude = 1, Mirror = 2, Boolean = 3 };
};

} // namespace elcad
