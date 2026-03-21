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

    // Detailed intersection that also returns triangle index and barycentric coords (u,v)
    bool rayIntersectDetailed(const QVector3D& origin, const QVector3D& dir,
                              float& outT, int& outTriIndex, float& outU, float& outV) const;

    // Accessors for CPU-side pick data (used for drawing per-triangle/vertex highlights)
    const std::vector<QVector3D>& pickVertices() const { return m_pickVerts; }
    const std::vector<unsigned int>& pickIndices() const { return m_pickIndices; }

    // Adjacency: triangle -> neighbor triangle indices
    // Each entry is a vector of triangle indices that share an edge with the triangle.
    const std::vector<std::vector<int>>& triangleNeighbors() const { return m_triNeighbors; }
    void ensureAdjacencyComputed();
    QVector3D triangleNormal(int triIdx) const; // unit normal computed from pickVerts/pickIndices

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

    // Cached triangle adjacency (triangle index -> neighbor triangle indices)
    std::vector<std::vector<int>> m_triNeighbors;

    bool m_initialized{false};
};

} // namespace elcad
