#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <QVector3D>
#include <vector>

#ifdef ELCAD_HAVE_OCCT
class TopoDS_Shape;
#endif

namespace elcad {

struct Vertex {
    QVector3D position;
    QVector3D normal;
};

// Holds OpenGL mesh data (triangles + edge lines) for one Body.
class MeshBuffer : protected QOpenGLFunctions_3_3_Core {
public:
    MeshBuffer()  = default;
    ~MeshBuffer();

    MeshBuffer(const MeshBuffer&)            = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;

#ifdef ELCAD_HAVE_OCCT
    // Tessellate the OCCT shape and upload to GPU.
    // deflection: chord deflection in mm (smaller = finer mesh)
    void build(const TopoDS_Shape& shape, float deflection = 0.01f);
#endif

    void drawTriangles();
    void drawEdges();

    bool isEmpty() const { return m_triCount == 0; }
    int  triangleCount() const { return m_triCount; }

    // Axis-aligned bounding box
    QVector3D bboxMin() const { return m_bboxMin; }
    QVector3D bboxMax() const { return m_bboxMax; }

    // Ray-mesh intersection (Möller-Trumbore). Returns true and sets t to hit distance.
    bool rayIntersect(const QVector3D& origin, const QVector3D& dir, float& t) const;

private:
    void upload(const std::vector<Vertex>& verts,
                const std::vector<unsigned int>& triIndices,
                const std::vector<unsigned int>& edgeIndices);
    void destroy();

    GLuint m_triVao{0}, m_triVbo{0}, m_triEbo{0};
    GLuint m_edgeVao{0}, m_edgeVbo{0}, m_edgeEbo{0};
    int    m_triCount{0};
    int    m_edgeIndexCount{0};

    QVector3D m_bboxMin{0,0,0};
    QVector3D m_bboxMax{0,0,0};

    // CPU-side data for ray picking
    std::vector<QVector3D>    m_pickVerts;
    std::vector<unsigned int> m_pickIndices;

    bool m_initialized{false};
};

} // namespace elcad
