#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <QString>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

namespace elcad {

class ShaderProgram : protected QOpenGLFunctions_3_3_Core {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    // Returns true on success; error text goes to qWarning
    bool load(const QString& vertPath, const QString& fragPath);

    void bind();
    void release();

    void setMat4(const char* name, const QMatrix4x4& m);
    void setMat3(const char* name, const QMatrix3x3& m);
    void setVec3(const char* name, const QVector3D& v);
    void setVec4(const char* name, const QVector4D& v);
    void setFloat(const char* name, float v);
    void setInt(const char* name, int v);

    bool isValid() const { return m_program != 0; }

private:
    GLuint compileShader(GLenum type, const QString& path);

    GLuint m_program{0};
};

} // namespace elcad
