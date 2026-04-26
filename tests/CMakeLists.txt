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
