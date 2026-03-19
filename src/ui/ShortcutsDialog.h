#pragma once
#include <QDialog>

namespace elcad {

class ShortcutsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutsDialog(QWidget* parent = nullptr);
};

} // namespace elcad
