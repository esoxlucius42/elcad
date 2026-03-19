#include "ui/ExtrudeDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

namespace elcad {

ExtrudeDialog::ExtrudeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Extrude");
    setFixedWidth(280);

    auto* root = new QVBoxLayout(this);

    auto* form = new QFormLayout;
    form->setSpacing(6);

    m_dist = new QDoubleSpinBox(this);
    m_dist->setRange(0.01, 100000.0);
    m_dist->setValue(10.0);
    m_dist->setSuffix(" mm");
    m_dist->setDecimals(2);
    form->addRow("Distance:", m_dist);

    m_mode = new QComboBox(this);
    m_mode->addItem("New Body");
    m_mode->addItem("Add to selected body");
    m_mode->addItem("Remove from selected body");
    form->addRow("Operation:", m_mode);

    m_sym = new QCheckBox("Symmetric (both directions)", this);
    form->addRow("", m_sym);

    root->addLayout(form);

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

ExtrudeParams ExtrudeDialog::params() const
{
    return { m_dist->value(), m_mode->currentIndex(), m_sym->isChecked() };
}

} // namespace elcad
