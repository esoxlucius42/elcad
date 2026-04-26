#include "SketchIntersectionFixtures.h"

#include <QCoreApplication>
#include <exception>
#include <functional>
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    struct RegressionCase {
        const char* name;
        std::function<void()> run;
    };

    const std::vector<RegressionCase> tests{
        {"selection", [] { elcad::test::runSketchIntersectionSelectionTests(); }},
        {"extrude", [] { elcad::test::runSketchIntersectionExtrudeTests(); }},
        {"refresh", [] { elcad::test::runSketchIntersectionRefreshTests(); }},
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
