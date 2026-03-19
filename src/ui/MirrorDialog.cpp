#include "ui/MirrorDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QDialogButtonBox>

namespace elcad {

MirrorDialog::MirrorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Mirror");

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    form->setSpacing(6);

    m_mirrorPlane = new QComboBox;
    m_mirrorPlane->addItem("XZ Plane (flip Y)");
    m_mirrorPlane->addItem("XY Plane (flip Z)");
    m_mirrorPlane->addItem("YZ Plane (flip X)");
    form->addRow("Mirror plane:", m_mirrorPlane);

    root->addLayout(form);

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
    setFixedWidth(280);
}

int MirrorDialog::mirrorPlane() const
{
    return m_mirrorPlane ? m_mirrorPlane->currentIndex() : 0;
}

} // namespace elcad
