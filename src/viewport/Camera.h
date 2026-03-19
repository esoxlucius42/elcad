#pragma once
#include <QMatrix4x4>
#include <QVector3D>
#include <QPoint>

namespace elcad {

class Camera {
public:
    Camera();

    // Called on viewport resize
    void setViewportSize(int w, int h);

    // Navigation — call from mouse/wheel events
    void orbitBegin(QPoint pos);
    void orbitMove(QPoint pos);

    void panBegin(QPoint pos);
    void panMove(QPoint pos);

    void zoom(float delta);  // positive = zoom in

    // Fit all — point camera at origin with some distance
    void fitAll();

    // Standard named views
    void setViewFront();
    void setViewRight();
    void setViewTop();
    void setViewIsometric();

    // Camera state
    QMatrix4x4 viewMatrix()       const;
    QMatrix4x4 projectionMatrix() const;
    QVector3D  position()         const;

    float nearPlane() const { return m_near; }
    float farPlane()  const { return m_far;  }
    float fov()       const { return m_fov;  }

    // Unproject screen pixel to world-space ray (origin + direction)
    void unprojectRay(int px, int py, int viewW, int viewH,
                      QVector3D& rayOrigin, QVector3D& rayDir) const;

    // Camera orientation accessors
    float yaw()   const { return m_yaw;   }
    float pitch() const { return m_pitch; }

    // Incremental orbit (used by NavCube arrows + drag)
    void orbitBy(float dyaw, float dpitch);

    // Direct snap to yaw/pitch (used by NavCube face clicks)
    void setYawPitch(float yaw, float pitch);

    // Toggle perspective / orthographic
    bool isPerspective() const { return m_perspective; }
    void setPerspective(bool on) { m_perspective = on; }

private:
    // Orbit target + spherical coords
    QVector3D m_target{0, 0, 0};
    float     m_radius{500.f};   // mm — distance from target
    float     m_yaw{45.f};       // degrees
    float     m_pitch{30.f};     // degrees

    int   m_viewW{800}, m_viewH{600};
    float m_fov{45.f};
    float m_near{1.f};
    float m_far{100000.f};

    bool m_perspective{true};

    // Drag state
    QPoint m_dragStart;
    float  m_dragYaw{0}, m_dragPitch{0};
    QVector3D m_dragTarget;
};

} // namespace elcad
