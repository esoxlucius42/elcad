add_executable(sketch_intersection_regression
    sketch/SketchIntersectionFixtures.cpp
    sketch/SketchIntersectionSelectionTest.cpp
    sketch/SketchIntersectionExtrudeTest.cpp
    sketch/SketchIntersectionRefreshTest.cpp
    sketch/SketchIntersectionRegressionMain.cpp
)

target_include_directories(sketch_intersection_regression PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sketch_intersection_regression PRIVATE elcad_core)

add_test(
    NAME sketch-intersection-regression
    COMMAND sketch_intersection_regression
)

add_executable(selection_regression
    selection/SelectionFixtures.cpp
    selection/AdditiveSelectionTest.cpp
    selection/EmptySpaceClearTest.cpp
    selection/ShortcutClearTest.cpp
    selection/SelectionRegressionMain.cpp
)

target_include_directories(selection_regression PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(selection_regression PRIVATE elcad_core)

add_test(
    NAME selection-regression
    COMMAND selection_regression
)
