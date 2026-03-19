#include "viewport/Grid.h"
#include <QDebug>

namespace elcad {

Grid::~Grid()
{
    if (m_vao) {
        initializeOpenGLFunctions();
        glDeleteVertexArrays(1, &m_vao);
    }
}

void Grid::initialize()
{
    if (m_initialized) return;
    initializeOpenGLFunctions();

    // Empty VAO — grid is drawn via gl_VertexID in the vertex shader
    glGenVertexArrays(1, &m_vao);

    m_shader.load(":/shaders/grid.vert", ":/shaders/grid.frag");
    m_initialized = true;
}

void Grid::render(const QMatrix4x4& view, const QMatrix4x4& proj,
                  float near_, float far_)
{
    if (!m_initialized || !m_shader.isValid()) return;

    QMatrix4x4 invVP = (proj * view).inverted();

    m_shader.bind();
    m_shader.setMat4("uInvViewProj", invVP);
    m_shader.setMat4("uView",        view);
    m_shader.setMat4("uProj",        proj);
    m_shader.setFloat("uNear",  near_);
    m_shader.setFloat("uFar",   far_);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    m_shader.release();
}

} // namespace elcad
