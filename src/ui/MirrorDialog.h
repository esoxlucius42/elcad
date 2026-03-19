#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace elcad {

class MirrorDialog : public QDialog {
    Q_OBJECT
public:
    explicit MirrorDialog(QWidget* parent = nullptr);

    int mirrorPlane() const;   // 0=XZ, 1=XY, 2=YZ

private:
    QComboBox* m_mirrorPlane{nullptr};
};

} // namespace elcad
