#include "viewport/Gizmo.h"
#include "core/Logger.h"
#include <QMatrix4x4>
#include <cmath>
#include <algorithm>

namespace elcad {

// ── Geometry constants (gizmo local space, 1.0 = desired screen size) ─────────
static constexpr float kShaftR    = 0.025f;  // arrow shaft radius
static constexpr float kShaftLen  = 0.65f;   // arrow shaft length
static constexpr float kHeadR     = 0.080f;  // cone base radius
static constexpr float kHeadLen   = 0.200f;  // cone height
static constexpr float kCubeHalf  = 0.055f;  // scale cube tip half-size
static constexpr float kRingR     = 0.80f;   // rotation ring major radius
static constexpr float kRingTubeR = 0.022f;  // rotation ring tube radius
static constexpr float kPlaneOff  = 0.25f;   // plane handle offset from origin
static constexpr float kPlaneSz   = 0.22f;   // plane handle square size

// Picking tolerances (wider than visual geometry for ease of use)
static constexpr float kPickShaftR  = 0.065f;
static constexpr float kPickRingIn  = kRingR - 0.14f;
static constexpr float kPickRingOut = kRingR + 0.14f;

static constexpr float kGizmoPx   = 120.0f; // desired screen-space size in pixels
static constexpr int   kArrowSegs = 12;
static constexpr int   kRingM     = 48;     // torus ring segments
static constexpr int   kRingN     = 8;      // torus tube segments

static constexpr float kPI = 3.14159265358979f;

// ── Geometry helpers ──────────────────────────────────────────────────────────

static void perpBasis(const QVector3D& n, QVector3D& u, QVector3D& v)
{
    if (qAbs(n.x()) < 0.9f)
        u = QVector3D::crossProduct(n, QVector3D(1, 0, 0)).normalized();
    else
        u = QVector3D::crossProduct(n, QVector3D(0, 1, 0)).normalized();
    v = QVector3D::crossProduct(n, u).normalized();
}

static QVector3D perp1(const QVector3D& axis)
{
    QVector3D u, v; perpBasis(axis, u, v); return u;
}

static QVector3D perp2(const QVector3D& axis)
{
    QVector3D u, v; perpBasis(axis, u, v); return v;
}

// ── Destructor ────────────────────────────────────────────────────────────────

Gizmo::~Gizmo()
{
    for (int i = 0; i < 6; ++i) destroyHandle(m_translateH[i]);
    for (int i = 0; i < 3; ++i) destroyHandle(m_rotateH[i]);
    for (int i = 0; i < 4; ++i) destroyHandle(m_scaleH[i]);
}

// ── Initialize ────────────────────────────────────────────────────────────────

void Gizmo::initialize()
{
    if (m_initialized) return;
    initializeOpenGLFunctions();

    m_shader.load(":/shaders/gizmo.vert", ":/shaders/gizmo.frag");

    buildTranslate();
    buildRotate();
    buildScale();

    m_initialized = true;
    LOG_INFO("Gizmo initialised — shader valid={}", m_shader.isValid());
}

// ── Geometry primitives ───────────────────────────────────────────────────────

void Gizmo::appendCylinder(std::vector<float>& v, std::vector<unsigned>& idx,
                             const QVector3D& from, const QVector3D& to,
                             float r, int segs)
{
    QVector3D dir = to - from;
    if (dir.length() < 1e-8f) return;
    dir.normalize();
    QVector3D u, vv; perpBasis(dir, u, vv);

    unsigned base = (unsigned)(v.size() / 3);

    for (int ring = 0; ring < 2; ++ring) {
        QVector3D ctr = (ring == 0) ? from : to;
        for (int i = 0; i < segs; ++i) {
            float a = 2.f * kPI * i / segs;
            QVector3D p = ctr + (u * std::cos(a) + vv * std::sin(a)) * r;
            v.push_back(p.x()); v.push_back(p.y()); v.push_back(p.z());
        }
    }

    // Tube side quads
    for (int i = 0; i < segs; ++i) {
        unsigned a = base + i,             b = base + (i + 1) % segs;
        unsigned c = base + segs + (i + 1) % segs, d = base + segs + i;
        idx.push_back(a); idx.push_back(b); idx.push_back(c);
        idx.push_back(a); idx.push_back(c); idx.push_back(d);
    }

    // Bottom cap
    unsigned cBot = (unsigned)(v.size() / 3);
    v.push_back(from.x()); v.push_back(from.y()); v.push_back(from.z());
    for (int i = 0; i < segs; ++i) {
        unsigned a = base + i, b = base + (i + 1) % segs;
        idx.push_back(cBot); idx.push_back(b); idx.push_back(a);
    }

    // Top cap
    unsigned cTop = (unsigned)(v.size() / 3);
    v.push_back(to.x()); v.push_back(to.y()); v.push_back(to.z());
    for (int i = 0; i < segs; ++i) {
        unsigned a = base + segs + i, b = base + segs + (i + 1) % segs;
        idx.push_back(cTop); idx.push_back(a); idx.push_back(b);
    }
}

void Gizmo::appendCone(std::vector<float>& v, std::vector<unsigned>& idx,
                        const QVector3D& base, const QVector3D& tip,
                        float r, int segs)
{
    QVector3D dir = (tip - base).normalized();
    QVector3D u, vv; perpBasis(dir, u, vv);

    unsigned bIdx = (unsigned)(v.size() / 3);
    for (int i = 0; i < segs; ++i) {
        float a = 2.f * kPI * i / segs;
        QVector3D p = base + (u * std::cos(a) + vv * std::sin(a)) * r;
        v.push_back(p.x()); v.push_back(p.y()); v.push_back(p.z());
    }

    unsigned tipIdx = (unsigned)(v.size() / 3);
    v.push_back(tip.x()); v.push_back(tip.y()); v.push_back(tip.z());

    unsigned capCtr = (unsigned)(v.size() / 3);
    v.push_back(base.x()); v.push_back(base.y()); v.push_back(base.z());

    for (int i = 0; i < segs; ++i) {
        unsigned a = bIdx + i, b = bIdx + (i + 1) % segs;
        idx.push_back(a); idx.push_back(b); idx.push_back(tipIdx);  // side
        idx.push_back(capCtr); idx.push_back(b); idx.push_back(a);  // base cap
    }
}

void Gizmo::appendBox(std::vector<float>& v, std::vector<unsigned>& idx,
                       const QVector3D& mn, const QVector3D& mx)
{
    unsigned base = (unsigned)(v.size() / 3);
    float cx[8] = {mn.x(),mx.x(),mx.x(),mn.x(), mn.x(),mx.x(),mx.x(),mn.x()};
    float cy[8] = {mn.y(),mn.y(),mx.y(),mx.y(), mn.y(),mn.y(),mx.y(),mx.y()};
    float cz[8] = {mn.z(),mn.z(),mn.z(),mn.z(), mx.z(),mx.z(),mx.z(),mx.z()};
    for (int i = 0; i < 8; ++i) {
        v.push_back(cx[i]); v.push_back(cy[i]); v.push_back(cz[i]);
    }
    unsigned faces[6][4] = {
        {0,1,2,3}, {7,6,5,4},
        {0,4,5,1}, {3,2,6,7},
        {0,3,7,4}, {1,5,6,2},
    };
    for (auto& f : faces) {
        idx.push_back(base+f[0]); idx.push_back(base+f[1]); idx.push_back(base+f[2]);
        idx.push_back(base+f[0]); idx.push_back(base+f[2]); idx.push_back(base+f[3]);
    }
}

void Gizmo::appendTorus(std::vector<float>& v, std::vector<unsigned>& idx,
                          const QVector3D& center, const QVector3D& normal,
                          float ringR, float tubeR, int ringS, int tubeS)
{
    QVector3D u, vv; perpBasis(normal, u, vv);
    unsigned base = (unsigned)(v.size() / 3);

    for (int i = 0; i < ringS; ++i) {
        float phi = 2.f * kPI * i / ringS;
        QVector3D ringPt = center + (u * std::cos(phi) + vv * std::sin(phi)) * ringR;
        QVector3D radial  = (u * std::cos(phi) + vv * std::sin(phi));
        for (int j = 0; j < tubeS; ++j) {
            float theta = 2.f * kPI * j / tubeS;
            QVector3D p = ringPt + (radial * std::cos(theta) + normal * std::sin(theta)) * tubeR;
            v.push_back(p.x()); v.push_back(p.y()); v.push_back(p.z());
        }
    }

    for (int i = 0; i < ringS; ++i) {
        int ni = (i + 1) % ringS;
        for (int j = 0; j < tubeS; ++j) {
            int nj = (j + 1) % tubeS;
            unsigned v0 = base + i *tubeS + j,   v1 = base + ni*tubeS + j;
            unsigned v2 = base + ni*tubeS + nj,  v3 = base + i *tubeS + nj;
            idx.push_back(v0); idx.push_back(v1); idx.push_back(v2);
            idx.push_back(v0); idx.push_back(v2); idx.push_back(v3);
        }
    }
}

void Gizmo::appendQuad(std::vector<float>& v, std::vector<unsigned>& idx,
                         const QVector3D p[4])
{
    unsigned base = (unsigned)(v.size() / 3);
    for (int i = 0; i < 4; ++i) {
        v.push_back(p[i].x()); v.push_back(p[i].y()); v.push_back(p[i].z());
    }
    // Draw both sides so the quad is visible from either direction
    idx.push_back(base); idx.push_back(base+1); idx.push_back(base+2);
    idx.push_back(base); idx.push_back(base+2); idx.push_back(base+3);
    idx.push_back(base); idx.push_back(base+2); idx.push_back(base+1);
    idx.push_back(base); idx.push_back(base+3); idx.push_back(base+2);
}

// ── Build geometry ────────────────────────────────────────────────────────────

void Gizmo::buildTranslate()
{
    static const QVector3D axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

    for (int i = 0; i < 3; ++i) {
        std::vector<float> v; std::vector<unsigned> idx;
        QVector3D dir = axes[i];
        QVector3D shaftEnd = dir * kShaftLen;
        QVector3D headEnd  = dir * (kShaftLen + kHeadLen);

        appendCylinder(v, idx, {0,0,0}, shaftEnd, kShaftR, kArrowSegs);
        appendCone    (v, idx, shaftEnd, headEnd, kHeadR, kArrowSegs);
        uploadHandle(m_translateH[i], v, idx);

        m_translateH[i].pickType = Handle::PickType::Capsule;
        m_translateH[i].capsA    = {0,0,0};
        m_translateH[i].capsB    = headEnd;
        m_translateH[i].capsR    = kPickShaftR;
    }

    // Plane quads: XY (i=0), XZ (i=1), YZ (i=2)
    struct { QVector3D a, b; } planes[3] = {
        { {1,0,0}, {0,1,0} },
        { {1,0,0}, {0,0,1} },
        { {0,1,0}, {0,0,1} },
    };

    for (int i = 0; i < 3; ++i) {
        std::vector<float> v; std::vector<unsigned> idx;
        QVector3D a = planes[i].a, b = planes[i].b;
        QVector3D corners[4] = {
            a * kPlaneOff + b * kPlaneOff,
            a * (kPlaneOff + kPlaneSz) + b * kPlaneOff,
            a * (kPlaneOff + kPlaneSz) + b * (kPlaneOff + kPlaneSz),
            a * kPlaneOff + b * (kPlaneOff + kPlaneSz),
        };
        appendQuad(v, idx, corners);
        uploadHandle(m_translateH[3 + i], v, idx);

        m_translateH[3 + i].pickType = Handle::PickType::Quad;
        for (int j = 0; j < 4; ++j)
            m_translateH[3 + i].quad[j] = corners[j];
    }
}

void Gizmo::buildRotate()
{
    static const QVector3D normals[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

    for (int i = 0; i < 3; ++i) {
        std::vector<float> v; std::vector<unsigned> idx;
        appendTorus(v, idx, {0,0,0}, normals[i], kRingR, kRingTubeR, kRingM, kRingN);
        uploadHandle(m_rotateH[i], v, idx);

        m_rotateH[i].pickType   = Handle::PickType::Ring;
        m_rotateH[i].ringNormal = normals[i];
        m_rotateH[i].ringInner  = kPickRingIn;
        m_rotateH[i].ringOuter  = kPickRingOut;
    }
}

void Gizmo::buildScale()
{
    static const QVector3D axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

    for (int i = 0; i < 3; ++i) {
        std::vector<float> v; std::vector<unsigned> idx;
        QVector3D dir = axes[i];
        QVector3D shaftEnd   = dir * kShaftLen;
        QVector3D cubeCenter = dir * (kShaftLen + kCubeHalf);

        appendCylinder(v, idx, {0,0,0}, shaftEnd, kShaftR, kArrowSegs);
        appendBox(v, idx,
                  cubeCenter - QVector3D(kCubeHalf, kCubeHalf, kCubeHalf),
                  cubeCenter + QVector3D(kCubeHalf, kCubeHalf, kCubeHalf));
        uploadHandle(m_scaleH[i], v, idx);

        m_scaleH[i].pickType = Handle::PickType::Capsule;
        m_scaleH[i].capsA    = {0,0,0};
        m_scaleH[i].capsB    = cubeCenter + dir * kCubeHalf;
        m_scaleH[i].capsR    = kPickShaftR;
    }

    // Center cube for uniform scale
    {
        std::vector<float> v; std::vector<unsigned> idx;
        float r = 0.07f;
        appendBox(v, idx, {-r,-r,-r}, {r,r,r});
        uploadHandle(m_scaleH[3], v, idx);

        m_scaleH[3].pickType = Handle::PickType::AABB;
        m_scaleH[3].aabbMin  = {-r,-r,-r};
        m_scaleH[3].aabbMax  = {r,r,r};
    }
}

// ── GPU upload / destroy ──────────────────────────────────────────────────────

void Gizmo::uploadHandle(Handle& h, const std::vector<float>& verts,
                          const std::vector<unsigned>& idx)
{
    if (verts.empty() || idx.empty()) return;

    glGenVertexArrays(1, &h.vao);
    glGenBuffers(1, &h.vbo);
    glGenBuffers(1, &h.ebo);

    glBindVertexArray(h.vao);

    glBindBuffer(GL_ARRAY_BUFFER, h.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, h.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idx.size() * sizeof(unsigned)),
                 idx.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    h.indexCount = (int)idx.size();
}

void Gizmo::destroyHandle(Handle& h)
{
    if (h.ebo) { glDeleteBuffers(1, &h.ebo); h.ebo = 0; }
    if (h.vbo) { glDeleteBuffers(1, &h.vbo); h.vbo = 0; }
    if (h.vao) { glDeleteVertexArrays(1, &h.vao); h.vao = 0; }
    h.indexCount = 0;
}

// ── Screen-space scale ────────────────────────────────────────────────────────

float Gizmo::computeScale(const QVector3D& camPos, int viewH, float fovDeg) const
{
    // TODO: support orthographic projection
    float dist = (camPos - m_position).length();
    if (dist < 1.0f) dist = 1.0f;
    float fovRad = qDegreesToRadians(fovDeg);
    return dist * std::tan(fovRad * 0.5f) * 2.0f * kGizmoPx / viewH;
}

// ── Colors ────────────────────────────────────────────────────────────────────

QVector4D Gizmo::handleColor(GizmoHandle h, bool hovered)
{
    if (hovered) return {1.0f, 0.85f, 0.05f, 1.0f};  // gold
    switch (h) {
    case GizmoHandle::X:   return {0.90f, 0.20f, 0.20f, 1.0f};
    case GizmoHandle::Y:   return {0.20f, 0.90f, 0.20f, 1.0f};
    case GizmoHandle::Z:   return {0.20f, 0.40f, 0.95f, 1.0f};
    case GizmoHandle::XY:  return {0.90f, 0.90f, 0.20f, 0.40f};
    case GizmoHandle::XZ:  return {0.90f, 0.20f, 0.90f, 0.40f};
    case GizmoHandle::YZ:  return {0.20f, 0.90f, 0.90f, 0.40f};
    case GizmoHandle::XYZ: return {0.95f, 0.95f, 0.95f, 1.0f};
    default: return {0.7f, 0.7f, 0.7f, 1.0f};
    }
}

QVector3D Gizmo::axisVec(GizmoHandle h)
{
    switch (h) {
    case GizmoHandle::X: return {1,0,0};
    case GizmoHandle::Y: return {0,1,0};
    case GizmoHandle::Z: return {0,0,1};
    default:             return {0,1,0};
    }
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void Gizmo::drawHandle(const Handle& h, const QVector4D& color, const QMatrix4x4& mvp)
{
    if (!h.vao || h.indexCount == 0) return;
    m_shader.setVec4("uColor", color);
    m_shader.setMat4("uMVP", mvp);
    glBindVertexArray(h.vao);
    glDrawElements(GL_TRIANGLES, h.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Gizmo::draw(const QMatrix4x4& view, const QMatrix4x4& proj,
                  const QVector3D& camPos, int viewH, float fovDeg)
{
    if (!m_visible || !m_initialized || !m_shader.isValid()) return;

    float s = computeScale(camPos, viewH, fovDeg);
    QMatrix4x4 model;
    model.translate(m_position);
    model.scale(s);
    QMatrix4x4 mvp = proj * view * model;

    // Clear depth so gizmo always draws on top of scene geometry
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.bind();

    auto col = [&](GizmoHandle h) {
        return handleColor(h, m_hovered == h || (m_dragging && m_dragHandle == h));
    };

    switch (m_mode) {
    case GizmoMode::Translate:
        // Axis arrows (X, Y, Z)
        for (int i = 0; i < 3; ++i) {
            auto h = static_cast<GizmoHandle>(static_cast<int>(GizmoHandle::X) + i);
            drawHandle(m_translateH[i], col(h), mvp);
        }
        // Plane quads (XY, XZ, YZ)
        for (int i = 0; i < 3; ++i) {
            auto h = static_cast<GizmoHandle>(static_cast<int>(GizmoHandle::XY) + i);
            drawHandle(m_translateH[3 + i], col(h), mvp);
        }
        break;

    case GizmoMode::Rotate:
        for (int i = 0; i < 3; ++i) {
            auto h = static_cast<GizmoHandle>(static_cast<int>(GizmoHandle::X) + i);
            drawHandle(m_rotateH[i], col(h), mvp);
        }
        break;

    case GizmoMode::Scale:
        for (int i = 0; i < 3; ++i) {
            auto h = static_cast<GizmoHandle>(static_cast<int>(GizmoHandle::X) + i);
            drawHandle(m_scaleH[i], col(h), mvp);
        }
        drawHandle(m_scaleH[3], col(GizmoHandle::XYZ), mvp);
        break;
    }

    m_shader.release();
}

// ── Picking ───────────────────────────────────────────────────────────────────

bool Gizmo::hitCapsule(const QVector3D& A, const QVector3D& B, float R,
                        const QVector3D& O, const QVector3D& D, float& t)
{
    QVector3D AB = B - A;
    QVector3D AO = O - A;
    float ab2 = AB.lengthSquared();
    if (ab2 < 1e-12f) {
        // Degenerate: sphere at A
        float b = 2.f * QVector3D::dotProduct(AO, D);
        float c = AO.lengthSquared() - R*R;
        float disc = b*b - 4*c;
        if (disc < 0) return false;
        t = (-b - std::sqrt(disc)) * 0.5f;
        return t >= 0;
    }

    float d  = QVector3D::dotProduct(D, AB);
    float e  = QVector3D::dotProduct(AO, AB);
    float f  = D.lengthSquared();
    float g  = QVector3D::dotProduct(AO, D);
    float den = f * ab2 - d * d;

    float s_seg, t_ray;
    if (qAbs(den) < 1e-10f) {
        s_seg = 0; t_ray = (f > 1e-10f) ? g / f : 0;
    } else {
        t_ray = (d * e - g * ab2) / den;
        s_seg = (d * t_ray + e) / ab2;
    }

    s_seg = qBound(0.f, s_seg, 1.f);
    QVector3D closeSeg = A + AB * s_seg;
    t_ray = QVector3D::dotProduct(closeSeg - O, D) / f;
    if (t_ray < 0) return false;

    float dist2 = (O + D * t_ray - closeSeg).lengthSquared();
    if (dist2 > R * R) return false;
    t = t_ray;
    return true;
}

bool Gizmo::hitAABB(const QVector3D& mn, const QVector3D& mx,
                     const QVector3D& O, const QVector3D& D, float& t)
{
    float tmin = 0.f, tmax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        float o  = (i==0) ? O.x()  : (i==1) ? O.y()  : O.z();
        float d  = (i==0) ? D.x()  : (i==1) ? D.y()  : D.z();
        float lo = (i==0) ? mn.x() : (i==1) ? mn.y() : mn.z();
        float hi = (i==0) ? mx.x() : (i==1) ? mx.y() : mx.z();

        if (qAbs(d) < 1e-8f) {
            if (o < lo || o > hi) return false;
        } else {
            float t1 = (lo - o) / d, t2 = (hi - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            tmin = qMax(tmin, t1);
            tmax = qMin(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    t = tmin;
    return t >= 0;
}

bool Gizmo::hitRing(const QVector3D& normal, float innerR, float outerR,
                     const QVector3D& O, const QVector3D& D, float& t)
{
    // Ring is centered at local origin (0,0,0) in local space
    float denom = QVector3D::dotProduct(D, normal);
    if (qAbs(denom) < 1e-6f) return false;
    t = QVector3D::dotProduct(-O, normal) / denom;
    if (t < 0) return false;
    float dist = (O + D * t).length();
    return dist >= innerR && dist <= outerR;
}

bool Gizmo::hitQuad(const QVector3D q[4], const QVector3D& O, const QVector3D& D, float& t)
{
    QVector3D e1 = q[1] - q[0];
    QVector3D e2 = q[3] - q[0];
    QVector3D n = QVector3D::crossProduct(e1, e2);
    float nLen = n.length();
    if (nLen < 1e-8f) return false;
    n /= nLen;

    float denom = QVector3D::dotProduct(D, n);
    if (qAbs(denom) < 1e-6f) return false;
    t = QVector3D::dotProduct(q[0] - O, n) / denom;
    if (t < 0) return false;

    QVector3D hit = O + D * t;
    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        if (QVector3D::dotProduct(QVector3D::crossProduct(q[j] - q[i], hit - q[i]), n) < 0)
            return false;
    }
    return true;
}

GizmoHandle Gizmo::pickAll(const QVector3D& O, const QVector3D& D) const
{
    float bestT = 1e30f;
    GizmoHandle best = GizmoHandle::None;
    float t;

    auto test = [&](GizmoHandle h, const Handle& hdl) {
        bool hit = false;
        switch (hdl.pickType) {
        case Handle::PickType::Capsule:
            hit = hitCapsule(hdl.capsA, hdl.capsB, hdl.capsR, O, D, t); break;
        case Handle::PickType::AABB:
            hit = hitAABB(hdl.aabbMin, hdl.aabbMax, O, D, t); break;
        case Handle::PickType::Ring:
            hit = hitRing(hdl.ringNormal, hdl.ringInner, hdl.ringOuter, O, D, t); break;
        case Handle::PickType::Quad:
            hit = hitQuad(hdl.quad, O, D, t); break;
        default: break;
        }
        if (hit && t > 0 && t < bestT) { bestT = t; best = h; }
    };

    switch (m_mode) {
    case GizmoMode::Translate:
        // Plane handles first (checked for visual priority but still pick closest)
        test(GizmoHandle::XY, m_translateH[3]);
        test(GizmoHandle::XZ, m_translateH[4]);
        test(GizmoHandle::YZ, m_translateH[5]);
        test(GizmoHandle::X,  m_translateH[0]);
        test(GizmoHandle::Y,  m_translateH[1]);
        test(GizmoHandle::Z,  m_translateH[2]);
        break;
    case GizmoMode::Rotate:
        test(GizmoHandle::X, m_rotateH[0]);
        test(GizmoHandle::Y, m_rotateH[1]);
        test(GizmoHandle::Z, m_rotateH[2]);
        break;
    case GizmoMode::Scale:
        test(GizmoHandle::XYZ, m_scaleH[3]);
        test(GizmoHandle::X,   m_scaleH[0]);
        test(GizmoHandle::Y,   m_scaleH[1]);
        test(GizmoHandle::Z,   m_scaleH[2]);
        break;
    }
    return best;
}

GizmoHandle Gizmo::pick(const QVector3D& rayO, const QVector3D& rayD,
                          const QVector3D& camPos, int viewH, float fovDeg)
{
    if (!m_visible) return GizmoHandle::None;
    float s = computeScale(camPos, viewH, fovDeg);
    QMatrix4x4 model; model.translate(m_position); model.scale(s);
    QMatrix4x4 inv = model.inverted();
    QVector4D lo = inv * QVector4D(rayO, 1.f);
    QVector4D ld = inv * QVector4D(rayD, 0.f);
    return pickAll({lo.x(),lo.y(),lo.z()}, {ld.x(),ld.y(),ld.z()});
}

bool Gizmo::updateHover(const QVector3D& rayO, const QVector3D& rayD,
                          const QVector3D& camPos, int viewH, float fovDeg)
{
    GizmoHandle newH = m_visible ? pick(rayO, rayD, camPos, viewH, fovDeg) : GizmoHandle::None;
    if (newH != m_hovered) { m_hovered = newH; return true; }
    return false;
}

// ── Drag helpers ──────────────────────────────────────────────────────────────

QVector3D Gizmo::rayOnAxis(const QVector3D& rayO, const QVector3D& rayD,
                             const QVector3D& axis, const QVector3D& camDir) const
{
    // Choose the helper plane (through m_position, containing axis) that is
    // most face-on to the camera for numerical stability.
    QVector3D p1 = perp1(axis), p2 = perp2(axis);
    QVector3D planeN = (qAbs(QVector3D::dotProduct(p1, camDir)) >
                        qAbs(QVector3D::dotProduct(p2, camDir))) ? p1 : p2;

    float denom = QVector3D::dotProduct(rayD, planeN);
    QVector3D hit;
    if (qAbs(denom) < 1e-6f) {
        float t2 = QVector3D::dotProduct(m_position - rayO, rayD);
        hit = rayO + rayD * t2;
    } else {
        float t2 = QVector3D::dotProduct(m_position - rayO, planeN) / denom;
        hit = rayO + rayD * t2;
    }
    float axisT = QVector3D::dotProduct(hit - m_position, axis);
    return m_position + axis * axisT;
}

QVector3D Gizmo::rayOnPlane(const QVector3D& rayO, const QVector3D& rayD,
                              const QVector3D& normal, const QVector3D& planePt) const
{
    float denom = QVector3D::dotProduct(rayD, normal);
    if (qAbs(denom) < 1e-6f) return planePt;
    float t = QVector3D::dotProduct(planePt - rayO, normal) / denom;
    return rayO + rayD * t;
}

// ── Drag begin / update / end ─────────────────────────────────────────────────

void Gizmo::beginDrag(GizmoHandle handle,
                       const QVector3D& rayO, const QVector3D& rayD,
                       const QVector3D& camPos, int viewH, float fovDeg)
{
    m_dragging       = true;
    m_dragHandle     = handle;
    m_dragGizmoScale = computeScale(camPos, viewH, fovDeg);
    QVector3D camDir = (m_position - camPos).normalized();

    if (m_mode == GizmoMode::Translate || m_mode == GizmoMode::Scale) {
        switch (handle) {
        case GizmoHandle::X:
        case GizmoHandle::Y:
        case GizmoHandle::Z:
            m_dragAxis    = axisVec(handle);
            m_dragStartT  = QVector3D::dotProduct(
                                rayOnAxis(rayO, rayD, m_dragAxis, camDir) - m_position,
                                m_dragAxis);
            break;
        case GizmoHandle::XY:
            m_dragPlaneNormal = {0,0,1};
            m_dragStartHit    = rayOnPlane(rayO, rayD, m_dragPlaneNormal, m_position);
            break;
        case GizmoHandle::XZ:
            m_dragPlaneNormal = {0,1,0};
            m_dragStartHit    = rayOnPlane(rayO, rayD, m_dragPlaneNormal, m_position);
            break;
        case GizmoHandle::YZ:
            m_dragPlaneNormal = {1,0,0};
            m_dragStartHit    = rayOnPlane(rayO, rayD, m_dragPlaneNormal, m_position);
            break;
        case GizmoHandle::XYZ:
            m_dragPlaneNormal = (m_position - camPos).normalized();
            m_dragStartHit    = rayOnPlane(rayO, rayD, m_dragPlaneNormal, m_position);
            break;
        default: break;
        }
    } else { // Rotate
        m_dragAxis        = axisVec(handle);
        m_dragPlaneNormal = m_dragAxis;
        QVector3D hit     = rayOnPlane(rayO, rayD, m_dragAxis, m_position);
        QVector3D local   = hit - m_position;
        m_dragStartAngle  = qRadiansToDegrees(std::atan2(
                                QVector3D::dotProduct(local, perp1(m_dragAxis)),
                                QVector3D::dotProduct(local, perp2(m_dragAxis))));
    }
}

Gizmo::DragDelta Gizmo::updateDrag(const QVector3D& rayO, const QVector3D& rayD,
                                    const QVector3D& camPos, int /*viewH*/, float /*fovDeg*/)
{
    DragDelta delta;
    if (!m_dragging) return delta;

    QVector3D camDir = (m_position - camPos).normalized();

    switch (m_mode) {
    case GizmoMode::Translate: {
        if (m_dragHandle == GizmoHandle::X ||
            m_dragHandle == GizmoHandle::Y ||
            m_dragHandle == GizmoHandle::Z) {
            QVector3D hitPt = rayOnAxis(rayO, rayD, m_dragAxis, camDir);
            float t = QVector3D::dotProduct(hitPt - m_position, m_dragAxis);
            delta.translate = m_dragAxis * (t - m_dragStartT);
        } else {
            QVector3D hitPt  = rayOnPlane(rayO, rayD, m_dragPlaneNormal, m_position);
            delta.translate  = hitPt - m_dragStartHit;
        }
        break;
    }

    case GizmoMode::Rotate: {
        QVector3D hitPt = rayOnPlane(rayO, rayD, m_dragPlaneNormal, m_position);
        QVector3D local = hitPt - m_position;
        float angle = qRadiansToDegrees(std::atan2(
                          QVector3D::dotProduct(local, perp1(m_dragAxis)),
                          QVector3D::dotProduct(local, perp2(m_dragAxis))));
        float da = angle - m_dragStartAngle;
        // Wrap to [-180, 180] to avoid discontinuity
        while (da >  180.f) da -= 360.f;
        while (da < -180.f) da += 360.f;
        delta.rotAxis  = m_dragAxis;
        delta.rotAngle = da;
        break;
    }

    case GizmoMode::Scale: {
        if (m_dragHandle == GizmoHandle::XYZ) {
            // Scale based on distance from gizmo center in the camera-facing plane
            QVector3D hitPt = rayOnPlane(rayO, rayD, m_dragPlaneNormal, m_position);
            float startDist = (m_dragStartHit - m_position).length();
            float currDist  = (hitPt - m_position).length();
            if (startDist > 1e-6f)
                delta.scaleFactor = currDist / startDist;
        } else {
            // Axis scale: map axis displacement to a uniform scale factor
            QVector3D hitPt = rayOnAxis(rayO, rayD, m_dragAxis, camDir);
            float t = QVector3D::dotProduct(hitPt - m_position, m_dragAxis);
            float norm = m_dragGizmoScale > 1e-6f ? m_dragGizmoScale : 1.f;
            delta.scaleFactor = 1.0f + (t - m_dragStartT) / norm;
            delta.scaleFactor = qMax(0.01f, delta.scaleFactor);
        }
        break;
    }
    }

    return delta;
}

void Gizmo::endDrag()
{
    m_dragging   = false;
    m_dragHandle = GizmoHandle::None;
}

} // namespace elcad
