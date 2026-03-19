#include "sketch/SketchPlane.h"
#include <QtMath>

#ifdef ELCAD_HAVE_OCCT
#include <BRep_Tool.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomLProp_SLProps.hxx>
#include <gp_Pln.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#endif

namespace elcad {

SketchPlane::SketchPlane()
    : SketchPlane(XY) {}

SketchPlane::SketchPlane(Standard s)
{
    switch (s) {
    case XZ:
        m_origin = {0, 0, 0};
        m_xAxis  = {1, 0, 0};
        m_yAxis  = {0, 0, 1};
        m_name   = "XZ";
        break;
    case XY:
        m_origin = {0, 0, 0};
        m_xAxis  = {1, 0, 0};
        m_yAxis  = {0, 1, 0};
        m_name   = "XY";
        break;
    case YZ:
        m_origin = {0, 0, 0};
        m_xAxis  = {0, 1, 0};
        m_yAxis  = {0, 0, 1};
        m_name   = "YZ";
        break;
    }
    m_normal = QVector3D::crossProduct(m_xAxis, m_yAxis).normalized();
}

SketchPlane::SketchPlane(QVector3D origin, QVector3D xAxis, QVector3D yAxis)
    : m_origin(origin)
    , m_xAxis(xAxis.normalized())
    , m_yAxis(yAxis.normalized())
    , m_normal(QVector3D::crossProduct(xAxis, yAxis).normalized())
    , m_name("Custom")
{}

SketchPlane SketchPlane::xz() { return SketchPlane(XZ); }
SketchPlane SketchPlane::xy() { return SketchPlane(XY); }
SketchPlane SketchPlane::yz() { return SketchPlane(YZ); }

#ifdef ELCAD_HAVE_OCCT
SketchPlane SketchPlane::fromFace(const TopoDS_Face& face)
{
    // Compute face centroid
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    gp_Pnt centroid = props.CentreOfMass();

    // Get face surface normal at centroid UV
    BRepAdaptor_Surface surf(face);
    double u = (surf.FirstUParameter() + surf.LastUParameter()) * 0.5;
    double v = (surf.FirstVParameter() + surf.LastVParameter()) * 0.5;

    GeomLProp_SLProps props2(surf.Surface().Surface(), u, v, 1, 1e-6);
    if (!props2.IsNormalDefined()) return SketchPlane(XZ);

    gp_Dir n = props2.Normal();
    if (face.Orientation() == TopAbs_REVERSED) n.Reverse();

    // Build an orthonormal frame
    gp_Dir xd = n.IsParallel(gp_Dir(0,0,1), 1e-3)
                ? gp_Dir(1,0,0)
                : gp_Dir(gp_Vec(n).Crossed(gp_Vec(0,0,1)).Normalized());
    gp_Dir yd = gp_Dir(gp_Vec(n).Crossed(gp_Vec(xd)));

    QVector3D origin((float)centroid.X(), (float)centroid.Y(), (float)centroid.Z());
    QVector3D xAxis((float)xd.X(), (float)xd.Y(), (float)xd.Z());
    QVector3D yAxis((float)yd.X(), (float)yd.Y(), (float)yd.Z());

    SketchPlane p(origin, xAxis, yAxis);
    p.m_name = "Face";
    return p;
}
#endif

QVector3D SketchPlane::to3D(float u, float v) const
{
    return m_origin + m_xAxis * u + m_yAxis * v;
}

QVector2D SketchPlane::to2D(QVector3D p) const
{
    QVector3D d = p - m_origin;
    return { QVector3D::dotProduct(d, m_xAxis),
             QVector3D::dotProduct(d, m_yAxis) };
}

float SketchPlane::intersectRay(QVector3D rayOrigin, QVector3D rayDir) const
{
    float denom = QVector3D::dotProduct(rayDir, m_normal);
    if (qAbs(denom) < 1e-6f) return -1.f;
    return QVector3D::dotProduct(m_origin - rayOrigin, m_normal) / denom;
}

} // namespace elcad
