#include "ui/NavCubeWidget.h"
#include <QMatrix4x4>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>

namespace elcad {

// ── Static data ───────────────────────────────────────────────────────────────

// Unit cube corners: index = XYZ bit-mask (0=neg, 1=pos), Z-up
//  0: (-,-,-)   1: (+,-,-)   2: (-,+,-)   3: (+,+,-)
//  4: (-,-,+)   5: (+,-,+)   6: (-,+,+)   7: (+,+,+)
const QVector3D NavCubeWidget::s_corners[8] = {
    {-0.5f, -0.5f, -0.5f}, // 0
    { 0.5f, -0.5f, -0.5f}, // 1
    {-0.5f,  0.5f, -0.5f}, // 2
    { 0.5f,  0.5f, -0.5f}, // 3
    {-0.5f, -0.5f,  0.5f}, // 4
    { 0.5f, -0.5f,  0.5f}, // 5
    {-0.5f,  0.5f,  0.5f}, // 6
    { 0.5f,  0.5f,  0.5f}, // 7
};

// Face: vertex indices (CCW when viewed from outside), label, snap yaw/pitch
// Normals (Z-up): Top=(0,0,+1), Bottom=(0,0,-1), Front=(0,+1,0), Back=(0,-1,0),
//                 Right=(+1,0,0), Left=(-1,0,0)
const NavCubeWidget::Face NavCubeWidget::s_faces[6] = {
    {{4, 6, 7, 5},  "Top",     0.f,    89.f},   // +Z
    {{0, 1, 3, 2},  "Bottom",  0.f,   -89.f},   // -Z
    {{2, 3, 7, 6},  "Front",   0.f,     0.f},   // +Y
    {{0, 4, 5, 1},  "Back",  180.f,     0.f},   // -Y
    {{1, 5, 7, 3},  "Right",  90.f,     0.f},   // +X
    {{0, 2, 6, 4},  "Left",  -90.f,     0.f},   // -X
};

// ── Constructor ───────────────────────────────────────────────────────────────

NavCubeWidget::NavCubeWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumSize(minimumSizeHint());
    setMaximumSize(sizeHint());
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

// ── Slot ─────────────────────────────────────────────────────────────────────

void NavCubeWidget::setOrientation(float yaw, float pitch, bool perspective)
{
    m_yaw         = yaw;
    m_pitch       = pitch;
    m_perspective = perspective;
    update();
}

// ── Projection helpers ────────────────────────────────────────────────────────

// Build a rotation matrix matching the camera: Z-up, yaw around Z, pitch around X.
// Returns world→screen rotation (just the rotation, no translation).
static QMatrix4x4 buildRotation(float yawDeg, float pitchDeg)
{
    float yawR   = qDegreesToRadians(yawDeg);
    float pitchR = qDegreesToRadians(pitchDeg);

    // Camera position direction (unit)
    float px = qCos(pitchR) * qSin(yawR);
    float py = qCos(pitchR) * qCos(yawR);
    float pz = qSin(pitchR);

    QMatrix4x4 m;
    m.setToIdentity();
    m.lookAt(QVector3D(px, py, pz), QVector3D(0, 0, 0), QVector3D(0, 0, 1));
    return m;
}

void NavCubeWidget::buildProjection(QPointF centre, float scale,
                                    std::array<QPointF, 8>& pts2d,
                                    std::array<float, 8>&   depthZ) const
{
    QMatrix4x4 rot = buildRotation(m_yaw, m_pitch);
    for (int i = 0; i < 8; ++i) {
        QVector3D tv = rot.map(s_corners[i]);
        // In camera space (OpenGL convention): camera looks along -Z.
        // tv.x() = screen right, tv.y() = screen up, tv.z() = depth
        // Negative Z = in front of camera. We use -tv.z() for "higher = closer to screen".
        pts2d[i] = {centre.x() + scale * tv.x(),
                    centre.y() - scale * tv.y()};  // flip Y for screen coords
        depthZ[i] = tv.z();  // more negative = closer to screen (in front)
    }
}

std::vector<NavCubeWidget::ProjectedFace>
NavCubeWidget::buildFaces(const std::array<QPointF, 8>& pts2d,
                           const std::array<float, 8>&   depthZ) const
{
    // World-space eye direction (unit vector from origin toward camera)
    float yawR   = qDegreesToRadians(m_yaw);
    float pitchR = qDegreesToRadians(m_pitch);
    QVector3D eyeDir(qCos(pitchR) * qSin(yawR),
                     qCos(pitchR) * qCos(yawR),
                     qSin(pitchR));

    // Face normals in world space
    static const QVector3D normals[6] = {
        { 0, 0, 1},   // Top
        { 0, 0,-1},   // Bottom
        { 0, 1, 0},   // Front
        { 0,-1, 0},   // Back
        { 1, 0, 0},   // Right
        {-1, 0, 0},   // Left
    };

    std::vector<ProjectedFace> faces(6);
    for (int fi = 0; fi < 6; ++fi) {
        const Face& f = s_faces[fi];
        ProjectedFace& pf = faces[fi];
        pf.faceIndex = fi;

        float avgZ = 0.f;
        for (int k = 0; k < 4; ++k) {
            int vi = f.verts[k];
            pf.pts[k] = pts2d[vi];
            avgZ += depthZ[vi];
        }
        pf.depth = avgZ * 0.25f;

        float dot = QVector3D::dotProduct(normals[fi], eyeDir);
        pf.brightness = dot;   // positive = facing camera
        pf.visible    = dot > -0.05f;  // show slightly-side faces too
    }

    // Sort back-to-front (most positive depth = furthest from camera → draw first)
    std::sort(faces.begin(), faces.end(),
              [](const ProjectedFace& a, const ProjectedFace& b){
                  return a.depth > b.depth;
              });
    return faces;
}

// ── Hit tests ────────────────────────────────────────────────────────────────

static bool pointInQuad(QPointF p, const std::array<QPointF, 4>& q)
{
    // Check if p is inside the convex quad using cross-product winding
    for (int i = 0; i < 4; ++i) {
        QPointF a = q[i], b = q[(i + 1) % 4];
        QPointF ab = b - a, ap = p - a;
        if (ab.x() * ap.y() - ab.y() * ap.x() < 0)
            return false;
    }
    return true;
}

int NavCubeWidget::hitTestFace(QPoint pos) const
{
    QPointF centre = widgetCentre();
    float   scale  = cubeScale();
    std::array<QPointF, 8> pts2d;
    std::array<float,   8> depthZ;
    buildProjection(centre, scale, pts2d, depthZ);
    auto faces = buildFaces(pts2d, depthZ);

    // Test front-to-back (reversed sort order) — first hit wins
    for (int i = (int)faces.size() - 1; i >= 0; --i) {
        const ProjectedFace& pf = faces[i];
        if (!pf.visible) continue;
        if (pointInQuad(pos, pf.pts))
            return pf.faceIndex;
    }
    return -1;
}

// Arrow regions: 4 triangles outside the cube bounding circle
int NavCubeWidget::hitTestArrow(QPoint pos) const
{
    QPointF c   = widgetCentre();
    float   r   = cubeScale() * 1.7f;   // inner radius
    float   ro  = cubeScale() * 2.3f;   // outer radius
    float   hw  = cubeScale() * 0.5f;   // half-width of triangle base

    // Arrow 0 = up, 1 = down, 2 = left, 3 = right
    // Each triangle: apex at outer radius, base at inner radius, half-width hw
    struct Arrow { QPointF apex, bl, br; };
    Arrow arrows[4] = {
        // Up
        {{c.x(),      c.y() - ro}, {c.x() - hw,  c.y() - r},  {c.x() + hw,  c.y() - r}},
        // Down
        {{c.x(),      c.y() + ro}, {c.x() + hw,  c.y() + r},  {c.x() - hw,  c.y() + r}},
        // Left
        {{c.x() - ro, c.y()},      {c.x() - r,   c.y() + hw}, {c.x() - r,   c.y() - hw}},
        // Right
        {{c.x() + ro, c.y()},      {c.x() + r,   c.y() - hw}, {c.x() + r,   c.y() + hw}},
    };

    for (int i = 0; i < 4; ++i) {
        auto& a = arrows[i];
        auto cross2 = [](QPointF o, QPointF a2, QPointF b2) {
            return (a2.x()-o.x())*(b2.y()-o.y()) - (a2.y()-o.y())*(b2.x()-o.x());
        };
        QPointF p(pos);
        float d0 = cross2(a.apex, a.bl,   p);
        float d1 = cross2(a.bl,   a.br,   p);
        float d2 = cross2(a.br,   a.apex, p);
        bool hasNeg = (d0 < 0) || (d1 < 0) || (d2 < 0);
        bool hasPos = (d0 > 0) || (d1 > 0) || (d2 > 0);
        if (!(hasNeg && hasPos))
            return i;
    }
    return -1;
}

bool NavCubeWidget::hitTestPerspButton(QPoint pos) const
{
    return perspButtonRect().contains(pos);
}

QRectF NavCubeWidget::perspButtonRect() const
{
    float sz = 22.f;
    return QRectF(width() - sz - 4, height() - sz - 4, sz, sz);
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void NavCubeWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background — transparent dark circle
    QPointF c   = widgetCentre();
    float   sc  = cubeScale();
    float   bgR = sc * 2.5f;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(40, 40, 40, 180));
    p.drawEllipse(c, bgR, bgR);

    // ── Arrow buttons ────────────────────────────────────────────────────────
    float r  = sc * 1.7f;
    float ro = sc * 2.3f;
    float hw = sc * 0.5f;

    // 0=up pitch+, 1=down pitch-, 2=left yaw-, 3=right yaw+
    struct Arrow { QPointF apex, bl, br; };
    Arrow arrows[4] = {
        {{c.x(),      c.y() - ro}, {c.x() - hw,  c.y() - r},  {c.x() + hw,  c.y() - r}},
        {{c.x(),      c.y() + ro}, {c.x() + hw,  c.y() + r},  {c.x() - hw,  c.y() + r}},
        {{c.x() - ro, c.y()},      {c.x() - r,   c.y() + hw}, {c.x() - r,   c.y() - hw}},
        {{c.x() + ro, c.y()},      {c.x() + r,   c.y() - hw}, {c.x() + r,   c.y() + hw}},
    };

    for (int i = 0; i < 4; ++i) {
        bool hov = (m_hoverArrow == i);
        p.setBrush(hov ? QColor(200, 200, 200, 220) : QColor(130, 130, 130, 180));
        p.setPen(Qt::NoPen);
        QPolygonF tri;
        tri << arrows[i].apex << arrows[i].bl << arrows[i].br;
        p.drawPolygon(tri);
    }

    // ── Cube faces ───────────────────────────────────────────────────────────
    std::array<QPointF, 8> pts2d;
    std::array<float,   8> depthZ;
    buildProjection(c, sc, pts2d, depthZ);
    auto faces = buildFaces(pts2d, depthZ);

    for (const auto& pf : faces) {
        float b = qBound(0.f, pf.brightness, 1.f);
        int   base = pf.visible ? int(55 + b * 90) : 35;
        bool  hov  = (m_hoverFace == pf.faceIndex && pf.visible);
        if (hov) base = qMin(base + 40, 230);

        QColor faceColor(base, base, base, 230);
        QColor edgeColor(qMin(base + 30, 255), qMin(base + 30, 255), qMin(base + 30, 255), 200);

        QPolygonF poly;
        for (auto& pt : pf.pts) poly << pt;

        p.setBrush(faceColor);
        p.setPen(QPen(edgeColor, 1.2));
        p.drawPolygon(poly);

        // Draw label on visible front-facing faces
        if (pf.visible && pf.brightness > 0.1f) {
            QPointF ctr;
            for (auto& pt : pf.pts) ctr += pt;
            ctr /= 4.0;

            float fontSize = qMax(sc * 0.28f, 7.f);
            QFont f = p.font();
            f.setPointSizeF(fontSize);
            f.setBold(true);
            p.setFont(f);

            // Text brightness
            int tc = int(180 + b * 75);
            p.setPen(QColor(tc, tc, tc));
            p.drawText(QRectF(ctr.x() - sc * 0.5, ctr.y() - sc * 0.25, sc, sc * 0.5),
                       Qt::AlignCenter, s_faces[pf.faceIndex].label);
        }
    }

    // ── Axis lines (from cube centre) ────────────────────────────────────────
    QMatrix4x4 rot = buildRotation(m_yaw, m_pitch);

    float axisLen = sc * 1.1f;
    auto drawAxis = [&](QVector3D dir, QColor col, const QString& lbl) {
        QVector3D tv = rot.map(dir);
        if (tv.z() < 0.f) {   // only draw axis lines that point toward the viewer
            QPointF end = {c.x() + axisLen * tv.x(), c.y() - axisLen * tv.y()};
            p.setPen(QPen(col, 2.0));
            p.drawLine(c, end);

            QFont f = p.font();
            f.setPointSizeF(qMax(sc * 0.22f, 6.f));
            f.setBold(false);
            p.setFont(f);
            p.setPen(col);
            QPointF lblPos = {c.x() + (axisLen + sc * 0.22f) * tv.x(),
                               c.y() - (axisLen + sc * 0.22f) * tv.y()};
            p.drawText(QRectF(lblPos.x() - 8, lblPos.y() - 8, 16, 16),
                       Qt::AlignCenter, lbl);
        }
    };

    drawAxis({1, 0, 0}, QColor(220, 60,  60),  "X");
    drawAxis({0, 1, 0}, QColor(60,  200, 60),  "Y");
    drawAxis({0, 0, 1}, QColor(60,  120, 220), "Z");

    // ── Perspective/ortho toggle button ──────────────────────────────────────
    QRectF pbr = perspButtonRect();
    p.setBrush(m_hoverPersp ? QColor(180, 180, 180, 240) : QColor(80, 80, 80, 200));
    p.setPen(QPen(QColor(160, 160, 160), 1));
    p.drawRoundedRect(pbr, 3, 3);

    // Draw a tiny cube icon inside the button
    float bx = pbr.center().x(), by = pbr.center().y();
    float bs = pbr.width() * 0.28f;
    if (m_perspective) {
        // Solid small cube (perspective mode indicator)
        p.setPen(QPen(QColor(220, 220, 220), 1));
        p.setBrush(QColor(160, 160, 160));
        p.drawRect(QRectF(bx - bs, by - bs, bs * 2, bs * 2));
    } else {
        // Wireframe cube (ortho mode indicator)
        p.setPen(QPen(QColor(220, 220, 220), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(bx - bs, by - bs, bs * 2, bs * 2));
        float o = bs * 0.5f;
        p.drawLine(QPointF(bx - bs, by - bs), QPointF(bx - bs + o, by - bs - o));
        p.drawLine(QPointF(bx + bs, by - bs), QPointF(bx + bs + o, by - bs - o));
        p.drawLine(QPointF(bx + bs, by + bs), QPointF(bx + bs + o, by + bs - o));
        p.drawLine(QPointF(bx + bs + o, by - bs - o), QPointF(bx + bs + o, by + bs - o));
        p.drawLine(QPointF(bx - bs + o, by - bs - o), QPointF(bx + bs + o, by - bs - o));
    }

    // Dropdown triangle next to button
    float tx = pbr.right() + 3, ty = pbr.center().y();
    QPolygonF dropTri;
    dropTri << QPointF(tx, ty - 3) << QPointF(tx + 6, ty - 3) << QPointF(tx + 3, ty + 3);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(160, 160, 160));
    p.drawPolygon(dropTri);
}

// ── Mouse events ─────────────────────────────────────────────────────────────

void NavCubeWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;

    if (hitTestPerspButton(e->pos())) {
        emit projectionToggleRequested();
        return;
    }

    int arrow = hitTestArrow(e->pos());
    if (arrow >= 0) {
        // 0=up pitch+45, 1=down pitch-45, 2=left yaw-45, 3=right yaw+45
        float dyaw = 0, dpitch = 0;
        switch (arrow) {
        case 0: dpitch =  45.f; break;
        case 1: dpitch = -45.f; break;
        case 2: dyaw   = -45.f; break;
        case 3: dyaw   =  45.f; break;
        }
        emit orbitRequested(dyaw, dpitch);
        return;
    }

    // Start drag (either on cube or background)
    m_dragging  = true;
    m_dragStart = e->pos();
}

void NavCubeWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;

    if (m_dragging) {
        QPoint delta = e->pos() - m_dragStart;
        if (delta.manhattanLength() < 4) {
            // Treat as click — check face
            int fi = hitTestFace(e->pos());
            if (fi >= 0) {
                emit viewPresetRequested(s_faces[fi].snapYaw, s_faces[fi].snapPitch);
            }
        }
        m_dragging = false;
    }
}

void NavCubeWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging) {
        // Per-frame deltas — update drag start each event to avoid accumulation bugs
        float dx = e->pos().x() - m_dragStart.x();
        float dy = e->pos().y() - m_dragStart.y();
        m_dragStart = e->pos();
        emit orbitRequested(dx * 0.5f, dy * 0.5f);
        return;
    }

    // Hover detection
    int prevFace  = m_hoverFace;
    int prevArrow = m_hoverArrow;
    bool prevPersp = m_hoverPersp;

    m_hoverPersp = hitTestPerspButton(e->pos());
    m_hoverArrow = m_hoverPersp ? -1 : hitTestArrow(e->pos());
    m_hoverFace  = (m_hoverArrow < 0 && !m_hoverPersp) ? hitTestFace(e->pos()) : -1;

    if (m_hoverFace != prevFace || m_hoverArrow != prevArrow || m_hoverPersp != prevPersp)
        update();
}

void NavCubeWidget::leaveEvent(QEvent*)
{
    m_hoverFace  = -1;
    m_hoverArrow = -1;
    m_hoverPersp = false;
    update();
}

} // namespace elcad
