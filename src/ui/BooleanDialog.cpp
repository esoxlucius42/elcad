#include "ui/BooleanDialog.h"
#include "document/Document.h"
#include "document/Body.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>

namespace elcad {

BooleanDialog::BooleanDialog(Document* doc, Operation op, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(op == Union ? "Boolean Union" : "Boolean Subtract");
    setFixedWidth(320);

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    form->setSpacing(6);

    m_target = new QComboBox(this);
    m_tool   = new QComboBox(this);

    for (auto& b : doc->bodies()) {
#ifdef ELCAD_HAVE_OCCT
        if (!b->hasShape()) continue;
#endif
        m_target->addItem(b->name(), static_cast<quint64>(b->id()));
        m_tool  ->addItem(b->name(), static_cast<quint64>(b->id()));
    }

    // Default: tool = second body
    if (m_tool->count() > 1) m_tool->setCurrentIndex(1);

    form->addRow("Target body (keep):", m_target);
    form->addRow(op == Subtract ? "Tool body (subtract):" : "Tool body (add):", m_tool);

    root->addLayout(form);
    root->addWidget(new QLabel("The tool body will be removed after the operation.", this));

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

quint64 BooleanDialog::targetBodyId() const
{
    return m_target->currentData().toULongLong();
}

quint64 BooleanDialog::toolBodyId() const
{
    return m_tool->currentData().toULongLong();
}

} // namespace elcad
