#pragma once
#include "viewport/ShaderProgram.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QMatrix4x4>

namespace elcad {

// Renders a small yellow cross + diamond at the world origin (0, 0, 0).
// Always visible so the user has a persistent spatial anchor.
class OriginMarker : protected QOpenGLFunctions_3_3_Core {
public:
    OriginMarker()  = default;
    ~OriginMarker();

    void initialize();
    void render(const QMatrix4x4& view, const QMatrix4x4& proj);

private:
    ShaderProgram m_shader;
    GLuint        m_vao{0};
    GLuint        m_vbo{0};
    int           m_vertexCount{0};
    bool          m_initialized{false};
};

} // namespace elcad
