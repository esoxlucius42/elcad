#include "ui/ToolOptionsPanel.h"
#include "document/Document.h"
#include "document/Body.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

namespace elcad {

ToolOptionsPanel::ToolOptionsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack);

    buildIdlePage();
    buildExtrudePage();
    buildMirrorPage();
    buildBooleanPage();
}

void ToolOptionsPanel::setDocument(Document* doc)
{
    m_doc = doc;
}

// ── Page builders ──────────────────────────────────────────────────────────────

void ToolOptionsPanel::buildIdlePage()
{
    auto* page   = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 12, 8, 12);

    auto* lbl = new QLabel("No tool selected", page);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet("color: #888; font-style: italic;");
    layout->addWidget(lbl);
    layout->addStretch();

    m_stack->addWidget(page); // index 0
}

void ToolOptionsPanel::buildExtrudePage()
{
    auto* page   = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* form = new QFormLayout;
    form->setSpacing(6);

    m_extrudeDist = new QDoubleSpinBox(page);
    m_extrudeDist->setRange(0.01, 100000.0);
    m_extrudeDist->setValue(10.0);
    m_extrudeDist->setSuffix(" mm");
    m_extrudeDist->setDecimals(2);
    form->addRow("Distance:", m_extrudeDist);

    m_extrudeMode = new QComboBox(page);
    m_extrudeMode->addItem("New Body");
    m_extrudeMode->addItem("Add to selected body");
    m_extrudeMode->addItem("Remove from selected body");
    form->addRow("Operation:", m_extrudeMode);

    m_extrudeSym = new QCheckBox("Symmetric (both directions)", page);
    form->addRow("", m_extrudeSym);

    layout->addLayout(form);
    layout->addStretch();

    auto* btnRow = new QHBoxLayout;
    auto* applyBtn  = new QPushButton("Apply", page);
    auto* cancelBtn = new QPushButton("Cancel", page);
    btnRow->addWidget(applyBtn);
    btnRow->addWidget(cancelBtn);
    layout->addLayout(btnRow);

    connect(applyBtn, &QPushButton::clicked, this, [this] {
        ExtrudeParams p;
        p.distance  = m_extrudeDist->value();
        p.mode      = m_extrudeMode->currentIndex();
        p.symmetric = m_extrudeSym->isChecked();
        showIdle();
        emit extrudeRequested(p);
    });
    connect(cancelBtn, &QPushButton::clicked, this, [this] {
        showIdle();
        emit cancelled();
    });

    m_stack->addWidget(page); // index 1
}

void ToolOptionsPanel::buildMirrorPage()
{
    auto* page   = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* form = new QFormLayout;
    form->setSpacing(6);

    m_mirrorPlane = new QComboBox(page);
    m_mirrorPlane->addItem("XZ Plane (flip Y)");
    m_mirrorPlane->addItem("XY Plane (flip Z)");
    m_mirrorPlane->addItem("YZ Plane (flip X)");
    form->addRow("Mirror plane:", m_mirrorPlane);

    layout->addLayout(form);
    layout->addStretch();

    auto* btnRow    = new QHBoxLayout;
    auto* applyBtn  = new QPushButton("Apply", page);
    auto* cancelBtn = new QPushButton("Cancel", page);
    btnRow->addWidget(applyBtn);
    btnRow->addWidget(cancelBtn);
    layout->addLayout(btnRow);

    connect(applyBtn, &QPushButton::clicked, this, [this] {
        int plane = m_mirrorPlane->currentIndex();
        showIdle();
        emit mirrorRequested(plane);
    });
    connect(cancelBtn, &QPushButton::clicked, this, [this] {
        showIdle();
        emit cancelled();
    });

    m_stack->addWidget(page); // index 2
}

void ToolOptionsPanel::buildBooleanPage()
{
    auto* page   = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* form = new QFormLayout;
    form->setSpacing(6);

    m_boolTarget = new QComboBox(page);
    m_boolTool   = new QComboBox(page);
    form->addRow("Target body (keep):", m_boolTarget);
    form->addRow("Tool body:", m_boolTool);

    layout->addLayout(form);
    layout->addWidget(new QLabel("The tool body is removed after the operation.", page));
    layout->addStretch();

    auto* btnRow    = new QHBoxLayout;
    m_boolApply     = new QPushButton("Apply", page);
    auto* cancelBtn = new QPushButton("Cancel", page);
    btnRow->addWidget(m_boolApply);
    btnRow->addWidget(cancelBtn);
    layout->addLayout(btnRow);

    connect(m_boolApply, &QPushButton::clicked, this, [this] {
        quint64 targetId = m_boolTarget->currentData().toULongLong();
        quint64 toolId   = m_boolTool->currentData().toULongLong();
        bool isSubtract  = m_boolIsSubtract;
        showIdle();
        if (isSubtract)
            emit booleanSubtractRequested(targetId, toolId);
        else
            emit booleanUnionRequested(targetId, toolId);
    });
    connect(cancelBtn, &QPushButton::clicked, this, [this] {
        showIdle();
        emit cancelled();
    });

    m_stack->addWidget(page); // index 3
}

// ── Page switching ────────────────────────────────────────────────────────────

void ToolOptionsPanel::showIdle()
{
    m_stack->setCurrentIndex(Page::Idle);
}

void ToolOptionsPanel::showExtrude()
{
    m_stack->setCurrentIndex(Page::Extrude);
}

void ToolOptionsPanel::showMirror()
{
    m_stack->setCurrentIndex(Page::Mirror);
}

void ToolOptionsPanel::showBooleanUnion()
{
    m_boolIsSubtract = false;
    populateBooleanCombos();
    m_stack->setCurrentIndex(Page::Boolean);
}

void ToolOptionsPanel::showBooleanSubtract()
{
    m_boolIsSubtract = true;
    populateBooleanCombos();
    m_stack->setCurrentIndex(Page::Boolean);
}

void ToolOptionsPanel::populateBooleanCombos()
{
    m_boolTarget->clear();
    m_boolTool->clear();

    if (!m_doc) return;

    for (auto& b : m_doc->bodies()) {
#ifdef ELCAD_HAVE_OCCT
        if (!b->hasShape()) continue;
#endif
        m_boolTarget->addItem(b->name(), static_cast<quint64>(b->id()));
        m_boolTool->addItem(b->name(), static_cast<quint64>(b->id()));
    }

    if (m_boolTool->count() > 1)
        m_boolTool->setCurrentIndex(1);
}

} // namespace elcad
