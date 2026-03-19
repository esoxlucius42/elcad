#include "viewport/Camera.h"
#include "core/Logger.h"
#include <QtMath>
#include <algorithm>

namespace elcad {

Camera::Camera()
{
    fitAll();
}

void Camera::setViewportSize(int w, int h)
{
    m_viewW = w;
    m_viewH = h;
}

// ── Orbit (right-mouse drag) ──────────────────────────────────────────────────

void Camera::orbitBegin(QPoint pos)
{
    m_dragStart = pos;
    m_dragYaw   = m_yaw;
    m_dragPitch = m_pitch;
}

void Camera::orbitMove(QPoint pos)
{
    float dx = pos.x() - m_dragStart.x();
    float dy = pos.y() - m_dragStart.y();

    m_yaw   = m_dragYaw   + dx * 0.4f;
    m_pitch = m_dragPitch + dy * 0.4f;  // positive dy (drag down) = pitch up
    m_pitch = std::clamp(m_pitch, -89.f, 89.f);
}

// ── Pan (middle-mouse drag) ───────────────────────────────────────────────────

void Camera::panBegin(QPoint pos)
{
    m_dragStart  = pos;
    m_dragTarget = m_target;
}

void Camera::panMove(QPoint pos)
{
    float dx = -(pos.x() - m_dragStart.x());
    float dy =  (pos.y() - m_dragStart.y());

    // Scale pan speed by radius so it feels consistent
    float speed = m_radius * 0.001f;

    QMatrix4x4 view = viewMatrix();
    QVector3D right(view(0,0), view(0,1), view(0,2));
    QVector3D up   (view(1,0), view(1,1), view(1,2));

    m_target = m_dragTarget + (right * dx + up * dy) * speed;
}

// ── Zoom (scroll wheel) ───────────────────────────────────────────────────────

void Camera::zoom(float delta)
{
    float factor = (delta > 0) ? 0.9f : 1.1f;
    m_radius = std::clamp(m_radius * factor, 1.f, 1e6f);
}

void Camera::orbitBy(float dyaw, float dpitch)
{
    m_yaw   += dyaw;
    m_pitch  = std::clamp(m_pitch + dpitch, -89.f, 89.f);
}

void Camera::setYawPitch(float yaw, float pitch)
{
    m_yaw   = yaw;
    m_pitch = std::clamp(pitch, -89.f, 89.f);
}

// ── Fit all ───────────────────────────────────────────────────────────────────

void Camera::fitAll()
{
    m_target = {0, 0, 0};
    m_radius = 500.f;
    m_yaw    = 45.f;
    m_pitch  = 30.f;
    LOG_DEBUG("Camera: fitAll — target=(0,0,0) radius=500 yaw=45 pitch=30");
}

void Camera::setViewFront()
{
    m_target = {0, 0, 0};
    m_yaw    = 0.f;
    m_pitch  = 0.f;
    LOG_DEBUG("Camera: view preset Front");
}

void Camera::setViewRight()
{
    m_target = {0, 0, 0};
    m_yaw    = 90.f;
    m_pitch  = 0.f;
    LOG_DEBUG("Camera: view preset Right");
}

void Camera::setViewTop()
{
    m_target = {0, 0, 0};
    m_yaw    = 0.f;
    m_pitch  = 89.f;
    LOG_DEBUG("Camera: view preset Top");
}

void Camera::setViewIsometric()
{
    m_target = {0, 0, 0};
    m_yaw    = 45.f;
    m_pitch  = 35.264f;  // arctan(1/sqrt(2)) — true isometric elevation
    LOG_DEBUG("Camera: view preset Isometric");
}

void Camera::unprojectRay(int px, int py, int viewW, int viewH,
                           QVector3D& rayOrigin, QVector3D& rayDir) const
{
    // NDC [-1,1]
    float ndcX =  (2.f * px)  / viewW - 1.f;
    float ndcY = -(2.f * py)  / viewH + 1.f;

    QMatrix4x4 invVP = (projectionMatrix() * viewMatrix()).inverted();

    QVector4D nearPt = invVP * QVector4D(ndcX, ndcY, -1.f, 1.f);
    QVector4D farPt  = invVP * QVector4D(ndcX, ndcY,  1.f, 1.f);

    nearPt /= nearPt.w();
    farPt  /= farPt.w();

    rayOrigin = nearPt.toVector3D();
    rayDir    = (farPt.toVector3D() - rayOrigin).normalized();
}

// ── Matrices ──────────────────────────────────────────────────────────────────

QVector3D Camera::position() const
{
    float yawR   = qDegreesToRadians(m_yaw);
    float pitchR = qDegreesToRadians(m_pitch);

    float x = m_radius * qCos(pitchR) * qSin(yawR);
    float y = m_radius * qCos(pitchR) * qCos(yawR);
    float z = m_radius * qSin(pitchR);  // Z is up

    return m_target + QVector3D(x, y, z);
}

QMatrix4x4 Camera::viewMatrix() const
{
    QMatrix4x4 m;
    m.lookAt(position(), m_target, QVector3D(0, 0, 1));
    return m;
}

QMatrix4x4 Camera::projectionMatrix() const
{
    QMatrix4x4 m;
    float aspect = (m_viewH > 0) ? float(m_viewW) / float(m_viewH) : 1.f;

    if (m_perspective) {
        m.perspective(m_fov, aspect, m_near, m_far);
    } else {
        float half = m_radius * qTan(qDegreesToRadians(m_fov * 0.5f));
        m.ortho(-half * aspect, half * aspect, -half, half, m_near, m_far);
    }
    return m;
}

} // namespace elcad
