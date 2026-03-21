#include "viewport/MeshBuffer.h"
#include "core/Logger.h"
#include <limits>
#include <unordered_map>
#include <array>
#include <algorithm>

#ifdef ELCAD_HAVE_OCCT
#include <BRep_Builder.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <TColgp_Array1OfDir.hxx>
#endif

namespace elcad {

MeshBuffer::~MeshBuffer()
{
    destroy();
}

void MeshBuffer::destroy()
{
    if (!m_initialized) return;
    initializeOpenGLFunctions();
    if (m_triVao)  { glDeleteVertexArrays(1, &m_triVao);  m_triVao  = 0; }
    if (m_triVbo)  { glDeleteBuffers(1, &m_triVbo);       m_triVbo  = 0; }
    if (m_triEbo)  { glDeleteBuffers(1, &m_triEbo);       m_triEbo  = 0; }
    if (m_edgeVao) { glDeleteVertexArrays(1, &m_edgeVao); m_edgeVao = 0; }
    if (m_edgeVbo) { glDeleteBuffers(1, &m_edgeVbo);      m_edgeVbo = 0; }
    if (m_edgeEbo) { glDeleteBuffers(1, &m_edgeEbo);      m_edgeEbo = 0; }
    m_triCount       = 0;
    m_edgeIndexCount = 0;
    m_initialized    = false;
}

#ifdef ELCAD_HAVE_OCCT

void MeshBuffer::build(const TopoDS_Shape& shape, float deflection)
{
    destroy();
    initializeOpenGLFunctions();
    m_initialized = true;

    // Tessellate
    BRepMesh_IncrementalMesh mesh(shape, deflection, Standard_False, 0.5, Standard_True);

    std::vector<Vertex>       verts;
    std::vector<unsigned int> triIndices;
    std::vector<unsigned int> edgeIndices;

    float fMin =  std::numeric_limits<float>::max();
    float fMax = -std::numeric_limits<float>::max();
    m_bboxMin = {fMin, fMin, fMin};
    m_bboxMax = {fMax, fMax, fMax};

    // ── Faces → triangles ────────────────────────────────────────────────────
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        TopLoc_Location    loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;

        bool reversed = (face.Orientation() == TopAbs_REVERSED);

        unsigned int baseIdx = static_cast<unsigned int>(verts.size());
        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : gp_Trsf(loc);

        // Build per-node normals by averaging face normals at each node
        // OCCT may or may not have pre-computed normals — compute them from triangles
        std::vector<gp_Vec> nodeNormals(tri->NbNodes(), gp_Vec(0,0,0));

        for (int t = 1; t <= tri->NbTriangles(); ++t) {
            Standard_Integer n1, n2, n3;
            tri->Triangle(t).Get(n1, n2, n3);
            if (reversed) std::swap(n2, n3);

            gp_Pnt p1 = tri->Node(n1).Transformed(trsf);
            gp_Pnt p2 = tri->Node(n2).Transformed(trsf);
            gp_Pnt p3 = tri->Node(n3).Transformed(trsf);

            gp_Vec e1(p1, p2), e2(p1, p3);
            gp_Vec fn = e1.Crossed(e2);
            nodeNormals[n1 - 1] += fn;
            nodeNormals[n2 - 1] += fn;
            nodeNormals[n3 - 1] += fn;
        }

        // Upload nodes as vertices
        for (int i = 1; i <= tri->NbNodes(); ++i) {
            gp_Pnt p = tri->Node(i).Transformed(trsf);
            gp_Vec n = nodeNormals[i - 1];
            double len = n.Magnitude();
            if (len > 1e-10) n /= len;

            Vertex v;
            v.position = {(float)p.X(), (float)p.Y(), (float)p.Z()};
            v.normal   = {(float)n.X(), (float)n.Y(), (float)n.Z()};
            verts.push_back(v);

            m_bboxMin.setX(std::min(m_bboxMin.x(), v.position.x()));
            m_bboxMin.setY(std::min(m_bboxMin.y(), v.position.y()));
            m_bboxMin.setZ(std::min(m_bboxMin.z(), v.position.z()));
            m_bboxMax.setX(std::max(m_bboxMax.x(), v.position.x()));
            m_bboxMax.setY(std::max(m_bboxMax.y(), v.position.y()));
            m_bboxMax.setZ(std::max(m_bboxMax.z(), v.position.z()));
        }

        // Triangle indices (1-based → 0-based)
        for (int t = 1; t <= tri->NbTriangles(); ++t) {
            Standard_Integer n1, n2, n3;
            tri->Triangle(t).Get(n1, n2, n3);
            if (reversed) std::swap(n2, n3);
            triIndices.push_back(baseIdx + n1 - 1);
            triIndices.push_back(baseIdx + n2 - 1);
            triIndices.push_back(baseIdx + n3 - 1);
        }
    }

    // ── Edges → lines ────────────────────────────────────────────────────────
    // Collect unique edge polylines from the triangulation polygon-on-triangulation
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(exp.Current());
        if (BRep_Tool::Degenerated(edge)) continue;

        TopLoc_Location loc;
        // Use discretised curve for edge lines
        BRepAdaptor_Curve curve(edge);
        GCPnts_TangentialDeflection disc;
        disc.Initialize(curve, deflection * 10.0, 0.1);

        if (disc.NbPoints() < 2) continue;

        unsigned int edgeBase = static_cast<unsigned int>(verts.size());
        for (int i = 1; i <= disc.NbPoints(); ++i) {
            gp_Pnt p = disc.Value(i);
            Vertex v;
            v.position = {(float)p.X(), (float)p.Y(), (float)p.Z()};
            v.normal   = {0.f, 1.f, 0.f};
            verts.push_back(v);
        }
        for (int i = 0; i < disc.NbPoints() - 1; ++i) {
            edgeIndices.push_back(edgeBase + i);
            edgeIndices.push_back(edgeBase + i + 1);
        }
    }

    upload(verts, triIndices, edgeIndices);

    LOG_DEBUG("MeshBuffer built — vertices={} triangles={} edge-segments={} "
              "bbox=({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f})",
              verts.size(), triIndices.size() / 3, edgeIndices.size() / 2,
              m_bboxMin.x(), m_bboxMin.y(), m_bboxMin.z(),
              m_bboxMax.x(), m_bboxMax.y(), m_bboxMax.z());

    if (triIndices.empty())
        LOG_WARN("MeshBuffer: tessellation produced no triangles — "
                 "shape may be degenerate, empty compound, or have no FACE topology");

    // Store CPU-side pick data (positions only, triangle indices only)
    m_pickVerts.clear();
    m_pickVerts.reserve(verts.size());
    for (const auto& v : verts)
        m_pickVerts.push_back(v.position);
    m_pickIndices = triIndices;

    // Reset adjacency; it will be computed on-demand
    m_triNeighbors.clear();
}

void MeshBuffer::ensureAdjacencyComputed()
{
    if (!m_triNeighbors.empty()) return;
    size_t triCount = m_pickIndices.size() / 3;
    m_triNeighbors.resize(triCount);

    // Edge -> list of triangle indices that reference it
    struct EdgeKey { unsigned int a,b; };
    struct EdgeKeyHash { size_t operator()(EdgeKey const& k) const noexcept { return (static_cast<size_t>(k.a) << 32) ^ static_cast<size_t>(k.b); } };
    struct EdgeKeyEq { bool operator()(EdgeKey const& x, EdgeKey const& y) const noexcept { return x.a == y.a && x.b == y.b; } };

    std::unordered_map<EdgeKey, std::vector<int>, EdgeKeyHash, EdgeKeyEq> edgeMap;
    edgeMap.reserve(triCount * 3);

    for (int tri = 0; tri < static_cast<int>(triCount); ++tri) {
        unsigned int i0 = m_pickIndices[tri * 3 + 0];
        unsigned int i1 = m_pickIndices[tri * 3 + 1];
        unsigned int i2 = m_pickIndices[tri * 3 + 2];
        std::array<std::pair<unsigned int,unsigned int>,3> edges = {{{i0,i1},{i1,i2},{i2,i0}}};
        for (auto e : edges) {
            unsigned int a = std::min(e.first, e.second);
            unsigned int b = std::max(e.first, e.second);
            EdgeKey k{a,b};
            edgeMap[k].push_back(tri);
        }
    }

    // Build neighbor lists from edge map
    for (const auto& kv : edgeMap) {
        const auto& tris = kv.second;
        for (size_t i = 0; i < tris.size(); ++i) {
            for (size_t j = i + 1; j < tris.size(); ++j) {
                int t1 = tris[i];
                int t2 = tris[j];
                m_triNeighbors[t1].push_back(t2);
                m_triNeighbors[t2].push_back(t1);
            }
        }
    }
}

QVector3D MeshBuffer::triangleNormal(int triIdx) const
{
    size_t base = static_cast<size_t>(triIdx) * 3;
    if (base + 2 >= m_pickIndices.size()) return QVector3D(0,0,0);
    const QVector3D& v0 = m_pickVerts[m_pickIndices[base+0]];
    const QVector3D& v1 = m_pickVerts[m_pickIndices[base+1]];
    const QVector3D& v2 = m_pickVerts[m_pickIndices[base+2]];
    QVector3D e1 = v1 - v0;
    QVector3D e2 = v2 - v0;
    QVector3D n = QVector3D::crossProduct(e1, e2);
    float len = std::sqrt(QVector3D::dotProduct(n,n));
    if (len > 1e-9f) return n / len;
    return QVector3D(0,0,0);
}

#endif // ELCAD_HAVE_OCCT

// ── Ray intersection ──────────────────────────────────────────────────────────

static bool rayAABB(const QVector3D& o, const QVector3D& d,
                    const QVector3D& bmin, const QVector3D& bmax, float& tMin)
{
    float tmax = std::numeric_limits<float>::max();
    tMin = -std::numeric_limits<float>::max();
    for (int i = 0; i < 3; ++i) {
        float inv = (d[i] != 0.f) ? 1.f / d[i] : std::numeric_limits<float>::max();
        float t1 = (bmin[i] - o[i]) * inv;
        float t2 = (bmax[i] - o[i]) * inv;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tmax = std::min(tmax, t2);
        if (tmax < tMin) return false;
    }
    return tmax > 0.f;
}

bool MeshBuffer::rayIntersectDetailed(const QVector3D& origin, const QVector3D& dir,
                                        float& outT, int& outTriIndex, float& outU, float& outV) const
{
    outTriIndex = -1;
    outU = outV = 0.f;
    if (m_pickIndices.empty()) return false;

    // Fast bbox rejection
    float bboxT;
    if (!rayAABB(origin, dir, m_bboxMin, m_bboxMax, bboxT)) return false;

    constexpr float kEps = 1e-7f;
    float minT = std::numeric_limits<float>::max();
    bool  hit  = false;

    const size_t n = m_pickIndices.size();
    int triIndex = 0;
    for (size_t i = 0; i + 2 < n; i += 3, ++triIndex) {
        const QVector3D& v0 = m_pickVerts[m_pickIndices[i]];
        const QVector3D& v1 = m_pickVerts[m_pickIndices[i+1]];
        const QVector3D& v2 = m_pickVerts[m_pickIndices[i+2]];

        QVector3D edge1 = v1 - v0;
        QVector3D edge2 = v2 - v0;
        QVector3D h = QVector3D::crossProduct(dir, edge2);
        float a = QVector3D::dotProduct(edge1, h);
        if (qAbs(a) < kEps) { continue; }

        float f = 1.f / a;
        QVector3D s = origin - v0;
        float u = f * QVector3D::dotProduct(s, h);
        if (u < 0.f || u > 1.f) continue;

        QVector3D q = QVector3D::crossProduct(s, edge1);
        float v = f * QVector3D::dotProduct(dir, q);
        if (v < 0.f || u + v > 1.f) continue;

        float t = f * QVector3D::dotProduct(edge2, q);
        if (t > kEps && t < minT) {
            minT = t;
            hit  = true;
            outU = u;
            outV = v;
            outTriIndex = triIndex;
        }
    }

    if (hit) outT = minT;
    return hit;
}

bool MeshBuffer::rayIntersect(const QVector3D& origin, const QVector3D& dir, float& outT) const
{
    int triIdx; float u,v;
    return rayIntersectDetailed(origin, dir, outT, triIdx, u, v);
}

void MeshBuffer::upload(const std::vector<Vertex>& verts,
                        const std::vector<unsigned int>& triIndices,
                        const std::vector<unsigned int>& edgeIndices)
{
    // ── Triangle VAO ─────────────────────────────────────────────────────────
    glGenVertexArrays(1, &m_triVao);
    glGenBuffers(1, &m_triVbo);
    glGenBuffers(1, &m_triEbo);

    glBindVertexArray(m_triVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_triVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(triIndices.size() * sizeof(unsigned int)),
                 triIndices.data(), GL_STATIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    // normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    m_triCount = static_cast<int>(triIndices.size());

    // ── Edge VAO ─────────────────────────────────────────────────────────────
    glGenVertexArrays(1, &m_edgeVao);
    glGenBuffers(1, &m_edgeVbo);
    glGenBuffers(1, &m_edgeEbo);

    glBindVertexArray(m_edgeVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_edgeVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_edgeEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(edgeIndices.size() * sizeof(unsigned int)),
                 edgeIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    m_edgeIndexCount = static_cast<int>(edgeIndices.size());
}

void MeshBuffer::drawTriangles()
{
    if (m_triCount == 0 || !m_triVao) return;
    glBindVertexArray(m_triVao);
    glDrawElements(GL_TRIANGLES, m_triCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void MeshBuffer::drawEdges()
{
    if (m_edgeIndexCount == 0 || !m_edgeVao) return;
    glBindVertexArray(m_edgeVao);
    glDrawElements(GL_LINES, m_edgeIndexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace elcad
