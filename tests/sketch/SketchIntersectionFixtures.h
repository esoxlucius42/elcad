#pragma once

#include "document/Document.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPicker.h"
#include <QVector2D>
#include <memory>
#include <string>
#include <vector>

namespace elcad::test {

std::unique_ptr<Sketch> makeRectangleCircleOverlapSketch();
std::unique_ptr<Sketch> makeNestedRectangleCircleSketch();
std::unique_ptr<Sketch> makeClockwiseArcSegmentSketch();
std::unique_ptr<Sketch> makeTangentRectangleCircleSketch();
std::unique_ptr<Sketch> makeRectangleWithDuplicateEdgeSketch();
std::unique_ptr<Sketch> makeOpenRectangleSketch();
std::unique_ptr<Document> makeCompletedOverlapDocument();

std::vector<SketchPicker::DerivedLoopTopology> deriveBoundedRegions(const Sketch& sketch);
int findSelectableRegionIndex(const std::vector<SketchPicker::DerivedLoopTopology>& topology,
                              QVector2D point);
SketchHit pickAt(Document& doc, QVector2D point);
float absoluteArea(const SketchPicker::DerivedLoopTopology& loop);
std::string describeBoundedRegions(const std::vector<SketchPicker::DerivedLoopTopology>& topology);

void require(bool condition, const std::string& message);
void requireNear(float actual, float expected, float tolerance, const std::string& message);

void runSketchIntersectionSelectionTests();
void runSketchIntersectionExtrudeTests();
void runSketchIntersectionRefreshTests();

} // namespace elcad::test
