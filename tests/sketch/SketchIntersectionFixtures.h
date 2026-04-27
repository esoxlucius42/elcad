#pragma once

#include "document/Document.h"
#include "sketch/Sketch.h"
#include "sketch/SketchPicker.h"
#include <QVector2D>
#include <memory>
#include <string>
#include <vector>

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#endif

class QOffscreenSurface;
class QOpenGLContext;

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

#ifdef ELCAD_HAVE_OCCT
class ScopedOffscreenGlContext {
public:
    ScopedOffscreenGlContext();
    ~ScopedOffscreenGlContext();

    bool isValid() const { return m_valid; }

private:
    std::unique_ptr<QOffscreenSurface> m_surface;
    std::unique_ptr<QOpenGLContext> m_context;
    bool m_valid{false};
};

std::unique_ptr<Document> makeSingleBodyDocument(const TopoDS_Shape& shape,
                                                 const QString& name = "Body");
TopoDS_Face faceByOrdinal(const TopoDS_Shape& shape, int faceOrdinal);
double faceArea(const TopoDS_Face& face);
double solidVolume(const TopoDS_Shape& shape);
#endif

void runSketchIntersectionSelectionTests();
void runSketchIntersectionExtrudeTests();
void runSketchIntersectionRefreshTests();

} // namespace elcad::test
