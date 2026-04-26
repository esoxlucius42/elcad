#include "app/Application.h"
#include "core/Logger.h"
#include "ui/MainWindow.h"
#include <QByteArray>
#include <QResource>
#include <cstdlib>

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(resources);

    // On Wayland, Qt6 uses EGL which returns EGL_BAD_MATCH when
    // QOpenGLWidget tries to create a desktop-GL context sharing with
    // the backing store's OpenGL ES context.  Force X11/XCB so that
    // Qt uses GLX, which supports desktop OpenGL 3.3 without issue.
    // Honour any explicit QT_QPA_PLATFORM the user has already set.
    bool waylandRedirected = false;
    if (qgetenv("QT_QPA_PLATFORM").isEmpty() &&
        !qgetenv("WAYLAND_DISPLAY").isEmpty())
    {
        qputenv("QT_QPA_PLATFORM", "xcb");
        waylandRedirected = true;
    }

    elcad::Application app(argc, argv);
    // Logger is initialised inside Application::Application() so we can
    // log from here onward.

    LOG_INFO("elcad v{} starting — Qt {}", app.applicationVersion().toStdString(), QT_VERSION_STR);
    if (waylandRedirected)
        LOG_INFO("Wayland detected — forced QT_QPA_PLATFORM=xcb for desktop-GL compatibility");
    else
        LOG_DEBUG("Platform: {}", qgetenv("QT_QPA_PLATFORM").constData());

    elcad::MainWindow win;
    win.show();

    const int exitCode = app.exec();
    LOG_INFO("elcad exiting with code {}", exitCode);
    spdlog::shutdown();
    return exitCode;
}
