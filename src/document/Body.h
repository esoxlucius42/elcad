#pragma once
#include <QString>
#include <QColor>
#include <QMatrix4x4>

// Forward-declare OCCT type without pulling in all headers
class TopoDS_Shape;

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Shape.hxx>
#endif

#include <memory>

namespace elcad {

class Body {
public:
    explicit Body(const QString& name = "Body");
    ~Body();

    // Non-copyable, movable
    Body(const Body&)            = delete;
    Body& operator=(const Body&) = delete;
    Body(Body&&)                 = default;
    Body& operator=(Body&&)      = default;

#ifdef ELCAD_HAVE_OCCT
    void            setShape(const TopoDS_Shape& shape);
    const TopoDS_Shape& shape() const;
    bool            hasShape() const;
#endif

    const QString& name()  const { return m_name; }
    void           setName(const QString& n) { m_name = n; }

    const QColor&  color() const { return m_color; }
    void           setColor(const QColor& c) { m_color = c; }

    bool           visible() const       { return m_visible; }
    void           setVisible(bool v)    { m_visible = v; }

    bool           selected() const      { return m_selected; }
    void           setSelected(bool s)   { m_selected = s; }

    // Unique ID (assigned at construction)
    quint64        id() const { return m_id; }

    // Mesh is dirty when shape changes and needs re-tessellation
    bool           meshDirty() const     { return m_meshDirty; }
    void           setMeshDirty(bool d)  { m_meshDirty = d; }

    // Bounding box — set by Renderer after tessellation
    QVector3D      bboxMin() const       { return m_bboxMin; }
    QVector3D      bboxMax() const       { return m_bboxMax; }
    bool           hasBbox() const       { return m_hasBbox; }
    void           setBbox(const QVector3D& mn, const QVector3D& mx) {
        m_bboxMin = mn; m_bboxMax = mx; m_hasBbox = true;
    }

    int            triangleCount() const         { return m_triCount; }
    void           setTriangleCount(int n)       { m_triCount = n; }

private:
    static quint64 s_nextId;
    quint64        m_id;
    QString        m_name;
    QColor         m_color{100, 160, 220};
    bool           m_visible{true};
    bool           m_selected{false};
    bool           m_meshDirty{true};
    bool           m_hasBbox{false};
    QVector3D      m_bboxMin{0,0,0};
    QVector3D      m_bboxMax{0,0,0};
    int            m_triCount{0};

#ifdef ELCAD_HAVE_OCCT
    std::unique_ptr<TopoDS_Shape> m_shape;
#endif
};

} // namespace elcad
