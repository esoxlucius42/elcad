#include "ui/PropertiesPanel.h"
#include "document/Document.h"
#include "document/Body.h"
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QColorDialog>
#include <QPixmap>

namespace elcad {

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack);
    root->addStretch();

    // ── Page 0: nothing selected ─────────────────────────────────────────────
    auto* nothingPage = new QWidget(m_stack);
    auto* nothingLay  = new QVBoxLayout(nothingPage);
    nothingLay->setContentsMargins(0, 0, 0, 0);
    auto* nothingLabel = new QLabel("No selection", nothingPage);
    nothingLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    QFont f = nothingLabel->font();
    f.setItalic(true);
    nothingLabel->setFont(f);
    nothingLay->addWidget(nothingLabel);
    nothingLay->addStretch();
    m_stack->addWidget(nothingPage);   // index 0

    // ── Page 1: body properties ───────────────────────────────────────────────
    auto* bodyPage = new QWidget(m_stack);
    auto* bodyLay  = new QVBoxLayout(bodyPage);
    bodyLay->setContentsMargins(0, 0, 0, 0);
    bodyLay->setSpacing(6);

    // Section title
    auto* titleLabel = new QLabel("Body", bodyPage);
    {
        QFont tf = titleLabel->font();
        tf.setBold(true);
        titleLabel->setFont(tf);
    }
    bodyLay->addWidget(titleLabel);

    // Separator line
    auto* line = new QFrame(bodyPage);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    bodyLay->addWidget(line);

    // Form
    auto* form = new QFormLayout;
    form->setSpacing(4);
    form->setContentsMargins(0, 0, 0, 0);

    m_nameEdit = new QLineEdit(bodyPage);
    form->addRow("Name:", m_nameEdit);

    m_colorBtn = new QPushButton(bodyPage);
    m_colorBtn->setFixedHeight(22);
    m_colorBtn->setFlat(true);
    m_colorBtn->setCursor(Qt::PointingHandCursor);
    form->addRow("Color:", m_colorBtn);

    m_idLabel = new QLabel(bodyPage);
    m_idLabel->setStyleSheet("color: #888;");
    form->addRow("ID:", m_idLabel);

    bodyLay->addLayout(form);

    // Separator
    auto* line2 = new QFrame(bodyPage);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    bodyLay->addWidget(line2);

    // Geometry section
    auto* geoLabel = new QLabel("Geometry", bodyPage);
    {
        QFont gf = geoLabel->font();
        gf.setBold(true);
        geoLabel->setFont(gf);
    }
    bodyLay->addWidget(geoLabel);

    auto* geoForm = new QFormLayout;
    geoForm->setSpacing(4);
    geoForm->setContentsMargins(0, 0, 0, 0);

    m_bboxLabel = new QLabel(bodyPage);
    m_bboxLabel->setWordWrap(true);
    geoForm->addRow("Size (mm):", m_bboxLabel);

    m_triLabel = new QLabel(bodyPage);
    geoForm->addRow("Triangles:", m_triLabel);

    bodyLay->addLayout(geoForm);
    bodyLay->addStretch();

    m_stack->addWidget(bodyPage);   // index 1

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_nameEdit, &QLineEdit::editingFinished,
            this, &PropertiesPanel::applyNameEdit);
    connect(m_colorBtn, &QPushButton::clicked,
            this, &PropertiesPanel::pickColor);

    showNothing();
}

void PropertiesPanel::setDocument(Document* doc)
{
    if (m_doc)
        disconnect(m_doc, nullptr, this, nullptr);

    m_doc = doc;

    if (m_doc)
        connect(m_doc, &Document::selectionChanged, this, &PropertiesPanel::refresh);

    refresh();
}

void PropertiesPanel::refresh()
{
    if (!m_doc) { showNothing(); return; }

    Body* sel = m_doc->singleSelectedBody();
    if (!sel) {
        // Check for multiple selection
        int count = 0;
        for (auto& b : m_doc->bodies())
            if (b->selected()) ++count;

        if (count == 0) { showNothing(); return; }

        // Multiple selected — show count only
        m_stack->setCurrentIndex(0);
        // Reuse nothing page label — just show count
        auto* lbl = qobject_cast<QLabel*>(
            m_stack->widget(0)->layout()->itemAt(0)->widget());
        if (lbl) lbl->setText(QString("%1 bodies selected").arg(count));
        return;
    }

    // Restore nothing label
    auto* lbl = qobject_cast<QLabel*>(
        m_stack->widget(0)->layout()->itemAt(0)->widget());
    if (lbl) lbl->setText("No selection");

    showBody(sel);
}

void PropertiesPanel::showBody(Body* body)
{
    m_currentBody = body;

    m_nameEdit->blockSignals(true);
    m_nameEdit->setText(body->name());
    m_nameEdit->blockSignals(false);

    // Color button
    QPixmap px(60, 16);
    px.fill(body->color());
    m_colorBtn->setIcon(QIcon(px));
    m_colorBtn->setText(body->color().name());

    m_idLabel->setText(QString::number(body->id()));

    if (body->hasBbox()) {
        QVector3D sz = body->bboxMax() - body->bboxMin();
        m_bboxLabel->setText(
            QString("%1 × %2 × %3")
                .arg(sz.x(), 0, 'f', 2)
                .arg(sz.y(), 0, 'f', 2)
                .arg(sz.z(), 0, 'f', 2));
        m_triLabel->setText(QString::number(body->triangleCount()));
    } else {
        m_bboxLabel->setText("—");
        m_triLabel->setText("—");
    }

    m_stack->setCurrentIndex(1);
}

void PropertiesPanel::showNothing()
{
    m_currentBody = nullptr;
    m_stack->setCurrentIndex(0);
}

void PropertiesPanel::applyNameEdit()
{
    if (!m_currentBody || !m_doc) return;
    QString newName = m_nameEdit->text().trimmed();
    if (newName.isEmpty() || newName == m_currentBody->name()) return;
    m_currentBody->setName(newName);
    emit m_doc->bodyChanged(m_currentBody);
}

void PropertiesPanel::pickColor()
{
    if (!m_currentBody || !m_doc) return;
    QColor c = QColorDialog::getColor(m_currentBody->color(), this, "Body Color");
    if (!c.isValid()) return;
    m_currentBody->setColor(c);
    // Update color button immediately
    QPixmap px(60, 16);
    px.fill(c);
    m_colorBtn->setIcon(QIcon(px));
    m_colorBtn->setText(c.name());
    emit m_doc->bodyChanged(m_currentBody);
}

} // namespace elcad
