#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
QT_END_NAMESPACE

namespace elcad {

struct ExtrudeParams {
    double distance{10.0};   // mm
    int    mode{0};          // 0=New, 1=Add, 2=Remove
    bool   symmetric{false}; // extrude both directions
};

class ExtrudeDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExtrudeDialog(QWidget* parent = nullptr);

    ExtrudeParams params() const;

private:
    QDoubleSpinBox* m_dist{nullptr};
    QComboBox*      m_mode{nullptr};
    QCheckBox*      m_sym{nullptr};
};

} // namespace elcad
