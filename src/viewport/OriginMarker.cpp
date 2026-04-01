#include "viewport/OriginMarker.h"
#include <QVector3D>
#include <array>

namespace elcad {

static constexpr float kArm     = 15.f;  // mm — cross arm half-length
static constexpr float kDiamond = 10.f;  // mm — diamond half-extent

OriginMarker::~OriginMarker()
{
    if (!m_initialized) return;
    initializeOpenGLFunctions();
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void OriginMarker::initialize()
{
    if (m_initialized) return;
    initializeOpenGLFunctions();

    m_shader.load(":/shaders/edge.vert", ":/shaders/edge.frag");

    // Cross + diamond in the XZ plane (matches the ground grid)
    const std::array<QVector3D, 20> verts = {{
        // Cross — X arm
        {-kArm, 0.f,    0.f},
        { kArm, 0.f,    0.f},
        // Cross — Z arm
        {  0.f, 0.f, -kArm},
        {  0.f, 0.f,  kArm},
        // Diamond
        { kDiamond, 0.f,      0.f},
        {      0.f, 0.f,  kDiamond},
        {      0.f, 0.f,  kDiamond},
        {-kDiamond, 0.f,      0.f},
        {-kDiamond, 0.f,      0.f},
        {      0.f, 0.f, -kDiamond},
        {      0.f, 0.f, -kDiamond},
        { kDiamond, 0.f,      0.f},
    }};
    m_vertexCount = static_cast<int>(verts.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 m_vertexCount * static_cast<GLsizeiptr>(sizeof(QVector3D)),
                 verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    m_initialized = true;
}

void OriginMarker::render(const QMatrix4x4& view, const QMatrix4x4& proj)
{
    if (!m_initialized || !m_shader.isValid()) return;

    QMatrix4x4 model; // identity — marker lives at world origin

    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    m_shader.bind();
    m_shader.setMat4("uModel",      model);
    m_shader.setMat4("uView",       view);
    m_shader.setMat4("uProjection", proj);
    m_shader.setVec3("uColor",      {1.0f, 1.0f, 0.0f}); // bright yellow

    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, m_vertexCount);
    glBindVertexArray(0);

    m_shader.release();
    glEnable(GL_DEPTH_TEST);
}

} // namespace elcad
