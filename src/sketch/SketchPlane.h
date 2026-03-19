#pragma once
#include <QVector2D>
#include <QVector3D>

#ifdef ELCAD_HAVE_OCCT
class TopoDS_Face;
#endif

namespace elcad {

// Defines a 2D coordinate frame embedded in 3D space.
// u → xAxis, v → yAxis, normal = xAxis × yAxis.
// Coordinates are in mm.
class SketchPlane {
public:
    enum Standard { XZ, XY, YZ };

    SketchPlane();                              // defaults to XZ (ground plane, Y=0)
    explicit SketchPlane(Standard s);
    SketchPlane(QVector3D origin, QVector3D xAxis, QVector3D yAxis);

    // Standard plane factories
    static SketchPlane xz();
    static SketchPlane xy();
    static SketchPlane yz();

#ifdef ELCAD_HAVE_OCCT
    // Build plane from an OCCT face (centroid + face normal)
    static SketchPlane fromFace(const TopoDS_Face& face);
#endif

    // 2D → 3D
    QVector3D to3D(float u, float v) const;
    QVector3D to3D(QVector2D p) const { return to3D(p.x(), p.y()); }

    // 3D → 2D (projection onto plane)
    QVector2D to2D(QVector3D p) const;

    // Ray–plane intersection: returns t where ray = origin + t*dir hits the plane.
    // Returns -1 if ray is parallel to the plane.
    float intersectRay(QVector3D rayOrigin, QVector3D rayDir) const;

    QVector3D origin() const { return m_origin; }
    QVector3D xAxis()  const { return m_xAxis; }
    QVector3D yAxis()  const { return m_yAxis; }
    QVector3D normal() const { return m_normal; }

    QString name() const { return m_name; }

private:
    QVector3D m_origin;
    QVector3D m_xAxis;
    QVector3D m_yAxis;
    QVector3D m_normal;
    QString   m_name;
};

} // namespace elcad
