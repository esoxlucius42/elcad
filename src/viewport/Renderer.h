#pragma once
#include "viewport/Camera.h"
#include "viewport/Gizmo.h"
#include "viewport/Grid.h"
#include "viewport/MeshBuffer.h"
#include "viewport/ShaderProgram.h"
#include "sketch/SketchRenderer.h"
#include <QOpenGLFunctions_3_3_Core>
#include <unordered_map>
#include <memory>

namespace elcad {

class Document;
class Body;
class Sketch;

// Owns OpenGL resources and orchestrates all draw calls.
class Renderer : protected QOpenGLFunctions_3_3_Core {
public:
    Renderer() = default;
    ~Renderer() = default;

    void initialize();
    void resize(int w, int h);
    void render(Camera& camera, Document* doc = nullptr,
                const std::vector<SketchEntity>* sketchPreview = nullptr,
                const QVector2D*                 snapPos       = nullptr);

    // Invalidate cached mesh for a body (call when body shape changes)
    void invalidateMesh(quint64 bodyId);
    void clearMeshCache();

    // Ray-pick: returns the closest visible body hit by the ray, or nullptr.
    Body* pickBody(const QVector3D& rayOrigin, const QVector3D& rayDir, Document* doc);

    bool gridVisible() const     { return m_gridVisible; }
    void setGridVisible(bool on) { m_gridVisible = on; }

    // Active sketch overlay (does not take ownership)
    void setActiveSketch(Sketch* sketch) { m_activeSketch = sketch; }

    // Gizmo access
    Gizmo& gizmo() { return m_gizmo; }

private:
    void drawBody(Body* body, const QMatrix4x4& view, const QMatrix4x4& proj,
                  const QVector3D& camPos);
    MeshBuffer* getMeshBuffer(Body* body);

    Grid           m_grid;
    ShaderProgram  m_phong;
    ShaderProgram  m_edge;
    SketchRenderer m_sketchRenderer;
    Gizmo          m_gizmo;
    Sketch*        m_activeSketch{nullptr};
    bool           m_initialized{false};
    bool           m_gridVisible{true};
    int            m_width{1}, m_height{1};

    // Body ID → mesh buffer cache
    std::unordered_map<quint64, std::unique_ptr<MeshBuffer>> m_meshCache;

    // Lighting constants
    QVector3D m_lightDir{0.6f, 1.0f, 0.8f};
    QVector3D m_lightColor{1.0f, 1.0f, 1.0f};
    float     m_ambient{0.15f};
};

} // namespace elcad
