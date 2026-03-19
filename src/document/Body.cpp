#include "document/Body.h"
#include "core/Logger.h"

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Shape.hxx>
#endif

namespace elcad {

quint64 Body::s_nextId = 1;

Body::Body(const QString& name)
    : m_id(s_nextId++)
    , m_name(name)
{
    LOG_DEBUG("Body created — id={} name='{}'", m_id, name.toStdString());
}

Body::~Body() = default;

#ifdef ELCAD_HAVE_OCCT

void Body::setShape(const TopoDS_Shape& shape)
{
    if (!m_shape)
        m_shape = std::make_unique<TopoDS_Shape>();
    *m_shape   = shape;
    m_meshDirty = true;
}

const TopoDS_Shape& Body::shape() const
{
    static TopoDS_Shape empty;
    return m_shape ? *m_shape : empty;
}

bool Body::hasShape() const
{
    return m_shape && !m_shape->IsNull();
}

#endif

} // namespace elcad
