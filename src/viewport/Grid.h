#pragma once
#include "viewport/ShaderProgram.h"
#include <QOpenGLFunctions_3_3_Core>

namespace elcad {

// Renders an infinite analytical grid on Y=0 with depth-correct fading.
// Requires glEnable(GL_BLEND) to be set by the caller.
class Grid : protected QOpenGLFunctions_3_3_Core {
public:
    Grid() = default;
    ~Grid();

    void initialize();
    void render(const QMatrix4x4& view,
                const QMatrix4x4& proj,
                float near_, float far_);

private:
    ShaderProgram m_shader;
    GLuint        m_vao{0};
    bool          m_initialized{false};
};

} // namespace elcad
