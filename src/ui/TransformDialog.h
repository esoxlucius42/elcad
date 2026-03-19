#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QComboBox;
class QTabWidget;
QT_END_NAMESPACE

namespace elcad {

// Shared dialog for Move / Rotate / Scale / Mirror.
class TransformDialog : public QDialog {
    Q_OBJECT
public:
    enum Mode { Move, Rotate, Scale, Mirror };

    explicit TransformDialog(Mode mode, QWidget* parent = nullptr);

    // Move
    double dx() const;
    double dy() const;
    double dz() const;

    // Rotate
    double axisX() const;
    double axisY() const;
    double axisZ() const;
    double originX() const;
    double originY() const;
    double originZ() const;
    double angleDeg() const;

    // Scale
    double scaleFactor() const;
    double scaleOX() const;
    double scaleOY() const;
    double scaleOZ() const;

    // Mirror
    int mirrorPlane() const;   // 0=XZ, 1=XY, 2=YZ

private:
    QDoubleSpinBox* m_dx{nullptr};
    QDoubleSpinBox* m_dy{nullptr};
    QDoubleSpinBox* m_dz{nullptr};
    QDoubleSpinBox* m_ax{nullptr};
    QDoubleSpinBox* m_ay{nullptr};
    QDoubleSpinBox* m_az{nullptr};
    QDoubleSpinBox* m_ox{nullptr};
    QDoubleSpinBox* m_oy{nullptr};
    QDoubleSpinBox* m_oz{nullptr};
    QDoubleSpinBox* m_angle{nullptr};
    QDoubleSpinBox* m_scale{nullptr};
    QDoubleSpinBox* m_scaleOX{nullptr};
    QDoubleSpinBox* m_scaleOY{nullptr};
    QDoubleSpinBox* m_scaleOZ{nullptr};
    QComboBox*      m_mirrorPlane{nullptr};
};

} // namespace elcad
