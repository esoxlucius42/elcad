#pragma once
#include "viewport/Camera.h"
#include "viewport/Gizmo.h"
#include "viewport/Grid.h"
#include "viewport/OriginMarker.h"
#include "viewport/MeshBuffer.h"
#include "viewport/ShaderProgram.h"
#include "sketch/SnapEngine.h"
#include "sketch/SketchRenderer.h"
#include "document/Document.h"
#include <QOpenGLFunctions_3_3_Core>
#include <unordered_map>
#include <memory>

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Face.hxx>
#endif

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
                const SnapResult*                snapResult    = nullptr,
                float                             devicePixelRatio = 1.0f);

    // Invalidate cached mesh for a body (call when body shape changes)
    void invalidateMesh(quint64 bodyId);
    void clearMeshCache();

    // Ray-pick: returns the closest visible body hit by the ray, or nullptr.
    Body* pickBody(const QVector3D& rayOrigin, const QVector3D& rayDir, Document* doc);

    // Ray-pick detailed: returns the closest visible hit (body/face/edge/vertex). If hit, outHit is filled and true returned.
    bool pickHit(const QVector3D& rayOrigin, const QVector3D& rayDir, Document* doc, Document::SelectedItem& outHit);

    // Return the face ordinal (0-based) for a given triangle index in a body, or -1 if unknown
    int faceOrdinalForTriangle(Body* body, int triIndex);

    // Expand a clicked triangle into a connected coplanar set. Returns triangle indices.
    std::vector<int> expandFaceSelection(Body* body, int startTri, float angleDeg = 10.0f, float distanceTol = 1e-3f);

#ifdef ELCAD_HAVE_OCCT
    // Build a TopoDS_Face from a set of mesh triangle indices. Returns a null face on failure.
    TopoDS_Face buildFaceFromTriangles(Body* body, const std::vector<int>& triIndices);
#endif

    bool gridVisible() const     { return m_gridVisible; }
    void setGridVisible(bool on) { m_gridVisible = on; }

    // Active sketch overlay (does not take ownership)
    void setActiveSketch(Sketch* sketch) { m_activeSketch = sketch; }

    // Hover state for completed sketch entities (set each frame from ViewportWidget).
    void setSketchHover(const Document::SelectedItem& item) {
        m_sketchHover      = item;
        m_hasSketchHover   = true;
    }
    void clearSketchHover() { m_hasSketchHover = false; }

    // Gizmo access
    Gizmo& gizmo() { return m_gizmo; }

private:
    void drawBody(Body* body, const QMatrix4x4& view, const QMatrix4x4& proj,
                  const QVector3D& camPos, Document* doc);
    MeshBuffer* getMeshBuffer(Body* body);


    Grid           m_grid;
    OriginMarker   m_originMarker;
    ShaderProgram  m_phong;
    ShaderProgram  m_edge;
    SketchRenderer m_sketchRenderer;
    Gizmo          m_gizmo;
    Sketch*        m_activeSketch{nullptr};
    bool           m_initialized{false};
    bool           m_gridVisible{true};
    int            m_width{1}, m_height{1};

    // Hover state for completed sketches (updated by ViewportWidget each mouse-move)
    Document::SelectedItem m_sketchHover;
    bool                   m_hasSketchHover{false};

    // Body ID → mesh buffer cache
    std::unordered_map<quint64, std::unique_ptr<MeshBuffer>> m_meshCache;

    // One-time GL buffers for highlights to avoid realloc each frame
    GLuint m_highlightVao{0};
    GLuint m_highlightVbo{0};
    // VBO for filled face highlights (positions only)
    GLuint m_faceHighlightVao{0};
    GLuint m_faceHighlightVbo{0};

    // Lighting constants
    QVector3D m_lightDir{0.6f, 1.0f, 0.8f};
    QVector3D m_lightColor{1.0f, 1.0f, 1.0f};
    // Hemisphere ambient
    QVector3D m_skyColor{0.55f, 0.65f, 0.80f};
    QVector3D m_groundColor{0.40f, 0.35f, 0.28f};
    // Fill/back light (diffuse only, negative Y so it also lights bottom faces)
    QVector3D m_fillDir{-0.5f, -0.3f, -0.6f};
    QVector3D m_fillColor{0.40f, 0.42f, 0.50f};
};

} // namespace elcad
