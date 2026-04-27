#include "SelectionFixtures.h"

#include <QApplication>
#include <exception>
#include <functional>
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    struct RegressionCase {
        const char* name;
        std::function<void()> run;
    };

    const std::vector<RegressionCase> tests{
        {"additive-selection", [] { elcad::test::runAdditiveSelectionTests(); }},
        {"empty-space-clear", [] { elcad::test::runEmptySpaceClearTests(); }},
        {"shortcut-clear", [] { elcad::test::runShortcutClearTests(); }},
    };

    int failed = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& ex) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": " << ex.what() << '\n';
        }
    }

    return failed == 0 ? 0 : 1;
}
