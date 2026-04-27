#include "SketchIntersectionFixtures.h"

#include "sketch/SketchPlane.h"
#ifdef ELCAD_HAVE_OCCT
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepCheck_Analyzer.hxx>
#endif
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace elcad::test {
namespace {

void addRectangle(Sketch& sketch, float minX, float minY, float maxX, float maxY)
{
    sketch.addLine(minX, minY, maxX, minY);
    sketch.addLine(maxX, minY, maxX, maxY);
    sketch.addLine(maxX, maxY, minX, maxY);
    sketch.addLine(minX, maxY, minX, minY);
}

} // namespace

std::unique_ptr<Sketch> makeRectangleCircleOverlapSketch()
{
    auto sketch = std::make_unique<Sketch>(SketchPlane::xy());
    addRectangle(*sketch, 0.0f, 0.0f, 20.0f, 20.0f);
    sketch->addCircle(0.0f, 0.0f, 10.0f);
    return sketch;
}

std::unique_ptr<Sketch> makeNestedRectangleCircleSketch()
{
    auto sketch = std::make_unique<Sketch>(SketchPlane::xy());
    addRectangle(*sketch, 0.0f, 0.0f, 30.0f, 30.0f);
    sketch->addCircle(15.0f, 15.0f, 6.0f);
    return sketch;
}

std::unique_ptr<Sketch> makeClockwiseArcSegmentSketch()
{
    auto sketch = std::make_unique<Sketch>(SketchPlane::xy());
    const float chordX = 5.0f;
    const float chordY = 10.0f * (std::sqrt(3.0f) * 0.5f);
    sketch->addArc(0.0f, 0.0f, 10.0f, 300.0f, 60.0f);
    sketch->addLine(chordX, -chordY, chordX, chordY);
    return sketch;
}

std::unique_ptr<Sketch> makeTangentRectangleCircleSketch()
{
    auto sketch = std::make_unique<Sketch>(SketchPlane::xy());
    addRectangle(*sketch, 0.0f, 0.0f, 20.0f, 20.0f);
    sketch->addCircle(30.0f, 10.0f, 10.0f);
    return sketch;
}

std::unique_ptr<Sketch> makeRectangleWithDuplicateEdgeSketch()
{
    auto sketch = std::make_unique<Sketch>(SketchPlane::xy());
    addRectangle(*sketch, 0.0f, 0.0f, 20.0f, 20.0f);
    sketch->addLine(0.0f, 0.0f, 20.0f, 0.0f);
    return sketch;
}

std::unique_ptr<Sketch> makeOpenRectangleSketch()
{
    auto sketch = std::make_unique<Sketch>(SketchPlane::xy());
    sketch->addLine(0.0f, 0.0f, 20.0f, 0.0f);
    sketch->addLine(20.0f, 0.0f, 20.0f, 20.0f);
    sketch->addLine(20.0f, 20.0f, 0.0f, 20.0f);
    return sketch;
}

std::unique_ptr<Document> makeCompletedOverlapDocument()
{
    auto doc = std::make_unique<Document>();
    Sketch* sketch = doc->beginSketch(SketchPlane::xy());
    addRectangle(*sketch, 0.0f, 0.0f, 20.0f, 20.0f);
    sketch->addCircle(0.0f, 0.0f, 10.0f);
    doc->endSketch();
    return doc;
}

std::vector<SketchPicker::DerivedLoopTopology> deriveBoundedRegions(const Sketch& sketch)
{
    return SketchPicker::buildLoopTopology(sketch);
}

int findSelectableRegionIndex(const std::vector<SketchPicker::DerivedLoopTopology>& topology,
                              QVector2D point)
{
    for (const auto& loop : topology) {
        if (SketchPicker::selectableLoopContainsPoint(topology, loop.loopIndex, point))
            return loop.loopIndex;
    }
    return -1;
}

SketchHit pickAt(Document& doc, QVector2D point)
{
    return SketchPicker::pick(QVector3D(point.x(), point.y(), 50.0f),
                              QVector3D(0.0f, 0.0f, -1.0f),
                              &doc);
}

float absoluteArea(const SketchPicker::DerivedLoopTopology& loop)
{
    return std::abs(loop.signedArea);
}

std::string describeBoundedRegions(const std::vector<SketchPicker::DerivedLoopTopology>& topology)
{
    std::ostringstream stream;
    stream << "regions=" << topology.size();
    for (const auto& loop : topology) {
        stream << " [idx=" << loop.loopIndex
               << " area=" << absoluteArea(loop)
               << " selectable=" << (loop.isSelectableMaterial ? "true" : "false")
               << " parent=";
        if (loop.parentLoopIndex.has_value())
            stream << *loop.parentLoopIndex;
        else
            stream << "none";
        stream << " points=" << loop.polygon.size() << "]";
    }
    return stream.str();
}

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(float actual, float expected, float tolerance, const std::string& message)
{
    if (std::abs(actual - expected) <= tolerance)
        return;

    std::ostringstream stream;
    stream << message << " (expected " << expected << ", got " << actual << ")";
    throw std::runtime_error(stream.str());
}

#ifdef ELCAD_HAVE_OCCT
ScopedOffscreenGlContext::ScopedOffscreenGlContext()
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);

    m_surface = std::make_unique<QOffscreenSurface>();
    m_surface->setFormat(format);
    m_surface->create();
    if (!m_surface->isValid())
        throw std::runtime_error("offscreen GL surface should be valid");

    m_context = std::make_unique<QOpenGLContext>();
    m_context->setFormat(m_surface->requestedFormat());
    if (!m_context->create())
        throw std::runtime_error("offscreen GL context creation should succeed");
    if (!m_context->makeCurrent(m_surface.get()))
        throw std::runtime_error("offscreen GL context should become current");

    m_valid = true;
}

ScopedOffscreenGlContext::~ScopedOffscreenGlContext()
{
    if (m_context)
        m_context->doneCurrent();
}

std::unique_ptr<Document> makeSingleBodyDocument(const TopoDS_Shape& shape, const QString& name)
{
    auto doc = std::make_unique<Document>();
    Body* body = doc->addBody(name);
    body->setShape(shape);
    return doc;
}

TopoDS_Face faceByOrdinal(const TopoDS_Shape& shape, int faceOrdinal)
{
    int index = 0;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next(), ++index) {
        if (index == faceOrdinal)
            return TopoDS::Face(exp.Current());
    }
    return TopoDS_Face{};
}

double faceArea(const TopoDS_Face& face)
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    return props.Mass();
}

double solidVolume(const TopoDS_Shape& shape)
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return props.Mass();
}
#endif

} // namespace elcad::test
