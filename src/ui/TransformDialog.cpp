#include "ui/TransformDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>

namespace elcad {

static QDoubleSpinBox* makeSpin(double lo, double hi, double val,
                                 QString suffix = " mm", int dec = 2)
{
    auto* s = new QDoubleSpinBox;
    s->setRange(lo, hi);
    s->setValue(val);
    s->setSuffix(suffix);
    s->setDecimals(dec);
    return s;
}

TransformDialog::TransformDialog(Mode mode, QWidget* parent)
    : QDialog(parent)
{
    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    form->setSpacing(6);

    switch (mode) {
    case Move:
        setWindowTitle("Move");
        m_dx = makeSpin(-1e6, 1e6, 0); form->addRow("ΔX:", m_dx);
        m_dy = makeSpin(-1e6, 1e6, 0); form->addRow("ΔY:", m_dy);
        m_dz = makeSpin(-1e6, 1e6, 0); form->addRow("ΔZ:", m_dz);
        break;

    case Rotate:
        setWindowTitle("Rotate");
        m_ax = makeSpin(-1,1, 0,"",3); m_ax->setValue(0); form->addRow("Axis X:", m_ax);
        m_ay = makeSpin(-1,1, 0,"",3); m_ay->setValue(0); form->addRow("Axis Y:", m_ay);
        m_az = makeSpin(-1,1, 1,"",3);                    form->addRow("Axis Z:", m_az);
        form->addRow(new QLabel("Origin:"));
        m_ox = makeSpin(-1e6,1e6,0); form->addRow("  OX:", m_ox);
        m_oy = makeSpin(-1e6,1e6,0); form->addRow("  OY:", m_oy);
        m_oz = makeSpin(-1e6,1e6,0); form->addRow("  OZ:", m_oz);
        m_angle = makeSpin(-360,360,90," °",1); form->addRow("Angle:", m_angle);
        break;

    case Scale:
        setWindowTitle("Scale");
        m_scale  = makeSpin(0.001,1000,1,"×",3); form->addRow("Factor:", m_scale);
        form->addRow(new QLabel("Center:"));
        m_scaleOX = makeSpin(-1e6,1e6,0); form->addRow("  X:", m_scaleOX);
        m_scaleOY = makeSpin(-1e6,1e6,0); form->addRow("  Y:", m_scaleOY);
        m_scaleOZ = makeSpin(-1e6,1e6,0); form->addRow("  Z:", m_scaleOZ);
        break;

    case Mirror:
        setWindowTitle("Mirror");
        m_mirrorPlane = new QComboBox;
        m_mirrorPlane->addItem("XZ Plane (flip Y)");
        m_mirrorPlane->addItem("XY Plane (flip Z)");
        m_mirrorPlane->addItem("YZ Plane (flip X)");
        form->addRow("Mirror plane:", m_mirrorPlane);
        break;
    }

    root->addLayout(form);

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
    setFixedWidth(280);
}

double TransformDialog::dx()        const { return m_dx     ? m_dx->value()     : 0; }
double TransformDialog::dy()        const { return m_dy     ? m_dy->value()     : 0; }
double TransformDialog::dz()        const { return m_dz     ? m_dz->value()     : 0; }
double TransformDialog::axisX()     const { return m_ax     ? m_ax->value()     : 0; }
double TransformDialog::axisY()     const { return m_ay     ? m_ay->value()     : 0; }
double TransformDialog::axisZ()     const { return m_az     ? m_az->value()     : 1; }
double TransformDialog::originX()   const { return m_ox     ? m_ox->value()     : 0; }
double TransformDialog::originY()   const { return m_oy     ? m_oy->value()     : 0; }
double TransformDialog::originZ()   const { return m_oz     ? m_oz->value()     : 0; }
double TransformDialog::angleDeg()  const { return m_angle  ? m_angle->value()  : 0; }
double TransformDialog::scaleFactor() const { return m_scale  ? m_scale->value()  : 1; }
double TransformDialog::scaleOX()   const { return m_scaleOX? m_scaleOX->value(): 0; }
double TransformDialog::scaleOY()   const { return m_scaleOY? m_scaleOY->value(): 0; }
double TransformDialog::scaleOZ()   const { return m_scaleOZ? m_scaleOZ->value(): 0; }
int    TransformDialog::mirrorPlane() const { return m_mirrorPlane ? m_mirrorPlane->currentIndex() : 0; }

} // namespace elcad
