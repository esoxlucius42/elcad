#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace elcad {

class Document;

class BooleanDialog : public QDialog {
    Q_OBJECT
public:
    enum Operation { Union, Subtract };

    explicit BooleanDialog(Document* doc, Operation op, QWidget* parent = nullptr);

    quint64 targetBodyId() const;
    quint64 toolBodyId()   const;

private:
    QComboBox* m_target{nullptr};
    QComboBox* m_tool{nullptr};
};

} // namespace elcad
