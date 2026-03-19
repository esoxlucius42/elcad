#pragma once
#include <QWidget>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QLineEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace elcad {

class Document;
class Body;

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void setDocument(Document* doc);

public slots:
    void refresh();

private:
    void showBody(Body* body);
    void showNothing();
    void applyNameEdit();
    void pickColor();

    Document* m_doc{nullptr};
    Body*     m_currentBody{nullptr};

    QStackedWidget* m_stack{nullptr};

    // Body info page (index 1)
    QLineEdit*   m_nameEdit{nullptr};
    QPushButton* m_colorBtn{nullptr};
    QLabel*      m_idLabel{nullptr};
    QLabel*      m_bboxLabel{nullptr};
    QLabel*      m_triLabel{nullptr};
};

} // namespace elcad
