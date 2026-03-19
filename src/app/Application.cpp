#include "app/Application.h"
#include "core/Logger.h"
#include <QPalette>
#include <QStyleFactory>

namespace elcad {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setApplicationName("elcad");
    setApplicationVersion("0.1.0");
    setOrganizationName("elcad");

    // Initialise the logger as early as possible so all subsequent code
    // (including applyDarkTheme and MainWindow construction) is covered.
    Logger::init(applicationDirPath().toStdString());

    applyDarkTheme();
}

void Application::applyDarkTheme()
{
    LOG_DEBUG("Applying dark Fusion theme");
    setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    // Window / base surfaces
    p.setColor(QPalette::Window,          QColor(30,  30,  30));
    p.setColor(QPalette::WindowText,      QColor(220, 220, 220));
    p.setColor(QPalette::Base,            QColor(22,  22,  22));
    p.setColor(QPalette::AlternateBase,   QColor(38,  38,  38));
    p.setColor(QPalette::ToolTipBase,     QColor(45,  45,  45));
    p.setColor(QPalette::ToolTipText,     QColor(220, 220, 220));
    p.setColor(QPalette::Text,            QColor(220, 220, 220));
    p.setColor(QPalette::Button,          QColor(45,  45,  45));
    p.setColor(QPalette::ButtonText,      QColor(220, 220, 220));
    p.setColor(QPalette::BrightText,      Qt::white);
    // Highlights
    p.setColor(QPalette::Highlight,       QColor(42, 130, 218));
    p.setColor(QPalette::HighlightedText, Qt::white);
    // Disabled state
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(90, 90, 90));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 90));
    p.setColor(QPalette::Disabled, QPalette::Highlight,  QColor(80, 80, 80));
    // Links
    p.setColor(QPalette::Link,            QColor(80, 160, 240));

    setPalette(p);

    // Extra QSS for fine-grained control
    setStyleSheet(R"(
        QMainWindow::separator { background: #1a1a1a; width: 2px; height: 2px; }
        QToolBar { background: #252525; border-bottom: 1px solid #111; spacing: 2px; padding: 2px; }
        QToolBar QToolButton { padding: 4px 6px; border-radius: 3px; }
        QToolBar QToolButton:hover  { background: #3a3a3a; }
        QToolBar QToolButton:checked { background: #2a6098; }
        QDockWidget::title { background: #1e1e1e; padding: 4px; font-weight: bold; }
        QDockWidget { titlebar-close-icon: none; }
        QTreeWidget { background: #1a1a1a; border: none; }
        QTreeWidget::item:selected { background: #2a6098; }
        QSplitter::handle { background: #111; }
        QStatusBar { background: #1a1a1a; color: #aaa; border-top: 1px solid #111; }
        QMenuBar { background: #252525; color: #ddd; }
        QMenuBar::item:selected { background: #3a3a3a; }
        QMenu { background: #2a2a2a; color: #ddd; border: 1px solid #111; }
        QMenu::item:selected { background: #2a6098; }
        QScrollBar:vertical { background: #1a1a1a; width: 8px; }
        QScrollBar::handle:vertical { background: #555; border-radius: 4px; min-height: 20px; }
        QLabel { color: #ddd; }
        QLineEdit { background: #1a1a1a; color: #ddd; border: 1px solid #444; border-radius: 3px; padding: 2px 4px; }
        QLineEdit:focus { border-color: #2a6098; }
    )");

    LOG_DEBUG("Dark Fusion theme applied");
}

} // namespace elcad
