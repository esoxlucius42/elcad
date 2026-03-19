#pragma once
#include "viewport/ShaderProgram.h"
#include "sketch/SketchEntity.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>

namespace elcad {

class Sketch;
class SketchPlane;

// Renders all sketch entities as 3D lines on the sketch plane.
// Uses a dynamic VBO updated each frame.
class SketchRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    SketchRenderer()  = default;
    ~SketchRenderer();

    void initialize();

    // Render sketch entities. previewEntities are drawn in cyan (in-progress tool).
    // snapPos (optional): if not null, draws a crosshair at the snap point.
    void render(const Sketch&                    sketch,
                const QMatrix4x4&               view,
                const QMatrix4x4&               proj,
                const std::vector<SketchEntity>& previewEntities = {},
                const QVector2D*                 snapPos         = nullptr);

private:
    // Collect 3D line segments with colour
    using LineList = std::vector<QVector3D>;

    void addEntityLines(const SketchEntity& e, const SketchPlane& plane,
                        LineList& normal, LineList& selected,
                        LineList& construction, LineList& preview,
                        bool isPreview) const;

    void addCircleLines(QVector2D center, float radius,
                        const SketchPlane& plane, LineList& out,
                        float startDeg = 0.f, float endDeg = 360.f) const;

    void drawLines(const LineList& pts, QVector3D color,
                   const QMatrix4x4& view, const QMatrix4x4& proj);

    ShaderProgram m_shader;
    GLuint        m_vao{0};
    GLuint        m_vbo{0};
    bool          m_initialized{false};
};

} // namespace elcad
