#pragma once
#include <QWidget>
#include <QVector3D>
#include <QPointF>
#include <array>

namespace elcad {

// A navigation cube widget rendered with QPainter.
// Shows a 3D projected cube that mirrors the main viewport's camera orientation.
// Supports: click face → snap to view, drag → orbit, arrow buttons → 45° steps,
// perspective/ortho toggle button.
class NavCubeWidget : public QWidget {
    Q_OBJECT
public:
    explicit NavCubeWidget(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {220, 220}; }
    QSize minimumSizeHint() const override { return {160, 160}; }

public slots:
    void setOrientation(float yaw, float pitch, bool perspective);

signals:
    void orbitRequested(float dyaw, float dpitch);
    void viewPresetRequested(float yaw, float pitch);
    void projectionToggleRequested();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    // ── Geometry ─────────────────────────────────────────────────────────────

    struct Face {
        int     verts[4];     // indices into m_corners8
        QString label;
        float   snapYaw;
        float   snapPitch;
    };

    struct ProjectedFace {
        std::array<QPointF, 4> pts;
        float   depth;        // average projected depth (for painter's algorithm)
        float   brightness;   // 0..1, how much the face faces the camera
        bool    visible;
        int     faceIndex;
    };

    // Build 8 projected cube corners from current yaw/pitch, centre = widget centre
    void buildProjection(QPointF centre, float scale,
                         std::array<QPointF, 8>& pts2d,
                         std::array<float, 8>&   depthZ) const;

    std::vector<ProjectedFace> buildFaces(const std::array<QPointF, 8>& pts2d,
                                          const std::array<float, 8>&   depthZ) const;

    // Hit-test: returns face index or -1
    int hitTestFace(QPoint pos) const;

    // Hit-test arrow regions: returns 0=up,1=down,2=left,3=right, or -1
    int hitTestArrow(QPoint pos) const;

    // Hit-test the persp/ortho toggle button
    bool hitTestPerspButton(QPoint pos) const;

    QRectF  perspButtonRect() const;
    QPointF widgetCentre()    const { return {width() * 0.5f, height() * 0.5f}; }
    float   cubeScale()       const { return qMin(width(), height()) * 0.21f; }

    // ── State ────────────────────────────────────────────────────────────────
    float m_yaw{45.f};
    float m_pitch{30.f};
    bool  m_perspective{true};

    // Drag state
    bool   m_dragging{false};
    QPoint m_dragStart;

    int m_hoverFace{-1};
    int m_hoverArrow{-1};
    bool m_hoverPersp{false};

    // ── Static face definitions ───────────────────────────────────────────────
    static const Face s_faces[6];
    // 8 unit-cube corners (X, Y, Z), Z-up, half-size 0.5
    static const QVector3D s_corners[8];
};

} // namespace elcad
