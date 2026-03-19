#include "viewport/ShaderProgram.h"
#include "core/Logger.h"
#include <QFile>

namespace elcad {

ShaderProgram::~ShaderProgram()
{
    if (m_program) {
        initializeOpenGLFunctions();
        glDeleteProgram(m_program);
    }
}

bool ShaderProgram::load(const QString& vertPath, const QString& fragPath)
{
    initializeOpenGLFunctions();

    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertPath);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragPath);
    if (!vert || !frag) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);

    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        LOG_ERROR("Shader program link failed — vert='{}' frag='{}' — linker output:\n{}",
                  vertPath.toStdString(), fragPath.toStdString(), log);
        glDeleteProgram(m_program);
        m_program = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return m_program != 0;
}

GLuint ShaderProgram::compileShader(GLenum type, const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("Cannot open shader file '{}' — check Qt resource path or file permissions",
                  path.toStdString());
        return 0;
    }
    QByteArray src = f.readAll();
    const char* ptr = src.constData();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);

        // Dump the first 25 lines of the offending shader source to help diagnose
        QList<QByteArray> lines = src.split('\n');
        std::string snippet;
        const int dumpLines = std::min(25, static_cast<int>(lines.size()));
        for (int i = 0; i < dumpLines; ++i)
            snippet += std::to_string(i + 1) + ": " + lines[i].toStdString() + "\n";

        LOG_ERROR("Shader compile failed — file='{}' type={} — compiler output:\n{}\n"
                  "--- source (first {} lines) ---\n{}",
                  path.toStdString(),
                  (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"),
                  log, dumpLines, snippet);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void ShaderProgram::bind()    { glUseProgram(m_program); }
void ShaderProgram::release() { glUseProgram(0); }

void ShaderProgram::setMat4(const char* name, const QMatrix4x4& m)
{
    GLint loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, m.constData());
}

void ShaderProgram::setMat3(const char* name, const QMatrix3x3& m)
{
    GLint loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniformMatrix3fv(loc, 1, GL_FALSE, m.constData());
}

void ShaderProgram::setVec3(const char* name, const QVector3D& v)
{
    GLint loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniform3f(loc, v.x(), v.y(), v.z());
}

void ShaderProgram::setVec4(const char* name, const QVector4D& v)
{
    GLint loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniform4f(loc, v.x(), v.y(), v.z(), v.w());
}

void ShaderProgram::setFloat(const char* name, float v)
{
    GLint loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniform1f(loc, v);
}

void ShaderProgram::setInt(const char* name, int v)
{
    GLint loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniform1i(loc, v);
}

} // namespace elcad
