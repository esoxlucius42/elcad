#pragma once
#include "viewport/ShaderProgram.h"
#include "sketch/SketchEntity.h"
#include "sketch/SnapEngine.h"
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
    // snapResult (optional): if not null, draws a crosshair + type indicator at the snap point.
    void render(const Sketch&                    sketch,
                const QMatrix4x4&               view,
                const QMatrix4x4&               proj,
                const std::vector<SketchEntity>& previewEntities = {},
                const SnapResult*                snapResult      = nullptr);

    // Render a completed (non-editable) sketch in a muted style.
    // hoveredEntityId: entity highlighted under the cursor (0 = none).
    // selectedEntityIds: entities selected by the user.
    // hoveredAreaIndex: polygon loop index hovered (-1 = none; -2 = circle area via entityId).
    // hoveredCircleEntityId: circle entity ID hovered as an area (0 = none).
    // selectedAreaIndices: loop indices that are selected (-1 entries ignored).
    // selectedCircleEntityIds: circle entity IDs selected as areas.
    void renderInactive(const Sketch&              sketch,
                        const QMatrix4x4&          view,
                        const QMatrix4x4&          proj,
                        quint64                    hoveredEntityId         = 0,
                        const std::vector<quint64>& selectedEntityIds      = {},
                        int                         hoveredAreaIndex       = -1,
                        quint64                     hoveredCircleEntityId  = 0,
                        const std::vector<int>&     selectedAreaIndices    = {},
                        const std::vector<quint64>& selectedCircleEntityIds = {});

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

    // Draw filled triangles (for area highlights).
    void drawTriangles(const std::vector<QVector3D>& pts, QVector3D color, float alpha,
                       const QMatrix4x4& view, const QMatrix4x4& proj);

    ShaderProgram m_shader;
    GLuint        m_vao{0};
    GLuint        m_vbo{0};
    bool          m_initialized{false};
};

} // namespace elcad

