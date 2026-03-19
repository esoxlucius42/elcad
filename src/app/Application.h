#pragma once
#include <QApplication>

namespace elcad {

class Application : public QApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv);

private:
    void applyDarkTheme();
};

} // namespace elcad
