#pragma once
#include "viewport/ShaderProgram.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <vector>

namespace elcad {

enum class GizmoMode   { Translate, Rotate, Scale, Extrude1D };
enum class GizmoHandle { None, X, Y, Z, XY, XZ, YZ, XYZ, Normal };

// Interactive 3D transform gizmo rendered in the viewport.
// Owned by Renderer; interacted with via ViewportWidget.
class Gizmo : protected QOpenGLFunctions_3_3_Core {
public:
    Gizmo()  = default;
    ~Gizmo();

    void initialize();

    // Draw all handles for the current mode.
    // Clears the depth buffer first so the gizmo always renders on top.
    void draw(const QMatrix4x4& view, const QMatrix4x4& proj,
              const QVector3D& camPos, int viewW, int viewH, float fovDeg);

    GizmoMode  mode()    const  { return m_mode; }
    void       setMode(GizmoMode m) { m_mode = m; }

    bool       visible()  const  { return m_visible; }
    void       setVisible(bool v){ m_visible = v; }

    QVector3D  position() const  { return m_position; }
    void       setPosition(const QVector3D& p) { m_position = p; }

    QVector3D  extrudeAxis() const { return m_extrudeAxis; }
    void       setExtrudeAxis(const QVector3D& axis) { m_extrudeAxis = axis.normalized(); }

    // Call each mouseMoveEvent; returns true if hover state changed (requires redraw).
    bool       updateHover(const QVector3D& rayO, const QVector3D& rayD,
                           const QVector3D& camPos, int viewW, int viewH, float fovDeg);
    GizmoHandle hoveredHandle() const { return m_hovered; }

    // Returns the handle under the given world-space ray, or None.
    GizmoHandle pick(const QVector3D& rayO, const QVector3D& rayD,
                     const QVector3D& camPos, int viewW, int viewH, float fovDeg);

    // Drag results: one field is populated per mode.
    struct DragDelta {
        QVector3D translate;          // Translate mode: world-space delta
        QVector3D rotAxis;            // Rotate mode: axis of rotation
        float     rotAngle{0};        // Rotate mode: angle in degrees (total from drag start)
        float     scaleFactor{1.0f};  // Scale mode: uniform scale factor (total from drag start)
    };

    void       beginDrag(GizmoHandle handle,
                         const QVector3D& rayO, const QVector3D& rayD,
                         const QVector3D& camPos, int viewW, int viewH, float fovDeg);
    DragDelta  updateDrag(const QVector3D& rayO, const QVector3D& rayD,
                          const QVector3D& camPos, int viewW, int viewH, float fovDeg);
    void       endDrag();

    bool        isDragging()  const { return m_dragging; }
    GizmoHandle dragHandle()  const { return m_dragHandle; }

private:
    // ── Per-handle GPU + picking data ───────────────────────────────────────
    struct Handle {
        GLuint vao{0}, vbo{0}, ebo{0};
        int    indexCount{0};

        enum class PickType { None, Capsule, AABB, Ring, Quad };
        PickType pickType{PickType::None};

        QVector3D capsA, capsB;    // capsule segment endpoints
        float     capsR{0};        // capsule radius

        QVector3D aabbMin, aabbMax;

        QVector3D ringNormal;      // ring plane normal
        float     ringInner{0}, ringOuter{0};

        QVector3D quad[4];         // quad corners
    };

    // Translate: slots 0-2 = X/Y/Z arrows, 3-5 = XY/XZ/YZ plane quads
    Handle m_translateH[6];
    // Rotate:    slots 0-2 = X/Y/Z rings
    Handle m_rotateH[3];
    // Scale:     slots 0-2 = X/Y/Z cube arrows, slot 3 = XYZ center cube
    Handle m_scaleH[4];
    // Extrude1D: single arrow along m_extrudeAxis (built along +Y in local space)
    Handle m_extrude1DH;

    void buildTranslate();
    void buildRotate();
    void buildScale();
    void buildExtrude1D();

    // ── Geometry primitives ─────────────────────────────────────────────────
    static void appendCylinder(std::vector<float>& v, std::vector<unsigned>& i,
                                const QVector3D& from, const QVector3D& to,
                                float r, int seg);
    static void appendCone(std::vector<float>& v, std::vector<unsigned>& i,
                            const QVector3D& base, const QVector3D& tip,
                            float r, int seg);
    static void appendBox(std::vector<float>& v, std::vector<unsigned>& i,
                          const QVector3D& mn, const QVector3D& mx);
    static void appendTorus(std::vector<float>& v, std::vector<unsigned>& i,
                             const QVector3D& center, const QVector3D& normal,
                             float ringR, float tubeR, int ringS, int tubeS);
    static void appendQuad(std::vector<float>& v, std::vector<unsigned>& i,
                            const QVector3D p[4]);

    void uploadHandle(Handle& h, const std::vector<float>& verts,
                      const std::vector<unsigned>& idx);
    void destroyHandle(Handle& h);

    // ── Screen-space scale ──────────────────────────────────────────────────
    float computeScale(const QVector3D& camPos, int viewW, int viewH, float fovDeg) const;

    // ── Picking ─────────────────────────────────────────────────────────────
    GizmoHandle pickAll(const QVector3D& localO, const QVector3D& localD) const;

    static bool hitCapsule(const QVector3D& A, const QVector3D& B, float R,
                            const QVector3D& O, const QVector3D& D, float& t);
    static bool hitAABB   (const QVector3D& mn, const QVector3D& mx,
                            const QVector3D& O, const QVector3D& D, float& t);
    static bool hitRing   (const QVector3D& normal,
                            float innerR, float outerR,
                            const QVector3D& O, const QVector3D& D, float& t);
    static bool hitQuad   (const QVector3D q[4],
                            const QVector3D& O, const QVector3D& D, float& t);

    // ── Drag helpers ────────────────────────────────────────────────────────
    // Project ray onto axis using a helper plane; returns world-space point on axis.
    QVector3D rayOnAxis(const QVector3D& rayO, const QVector3D& rayD,
                        const QVector3D& axis, const QVector3D& camDir) const;
    // Intersect ray with plane through planePt with given normal.
    QVector3D rayOnPlane(const QVector3D& rayO, const QVector3D& rayD,
                         const QVector3D& normal, const QVector3D& planePt) const;

    // ── Draw helpers ────────────────────────────────────────────────────────
    void drawHandle(const Handle& h, const QVector4D& color,
                    const QMatrix4x4& mvp);
    static QVector4D handleColor(GizmoHandle h, bool hovered);
    static QVector3D axisVec(GizmoHandle h);

    // ── State ───────────────────────────────────────────────────────────────
    ShaderProgram m_shader;
    bool          m_initialized{false};

    GizmoMode   m_mode    {GizmoMode::Translate};
    QVector3D   m_position{0, 0, 0};
    QVector3D   m_extrudeAxis{0, 1, 0};  // for Extrude1D mode
    bool        m_visible {false};
    GizmoHandle m_hovered {GizmoHandle::None};

    bool        m_dragging    {false};
    GizmoHandle m_dragHandle  {GizmoHandle::None};
    QVector3D   m_dragAnchorPos;          // m_position snapshot at drag start (used by Extrude1D)
    QVector3D   m_dragAxis;           // world-space drag axis (for axis + rotate drag)
    QVector3D   m_dragPlaneNormal;    // drag plane normal
    QVector3D   m_dragStartHit;       // world-space hit at drag start (plane drags)
    float       m_dragStartT   {0};   // axis parameter at drag start
    float       m_dragStartAngle{0};  // angle at drag start (rotate)
    float       m_dragGizmoScale{1};  // gizmo world-space size at drag start
};

} // namespace elcad
