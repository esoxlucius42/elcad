#pragma once
#include <QDockWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLabel>

namespace elcad {

class Document;
class Body;

class BodyListPanel : public QWidget {
    Q_OBJECT
public:
    explicit BodyListPanel(QWidget* parent = nullptr);

    void setDocument(Document* doc);

signals:
    void bodySelectionChanged(quint64 bodyId, bool selected);

private slots:
    void onBodyAdded(Body* body);
    void onBodyRemoved(quint64 id);
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onSelectionChanged();
    void onDocumentSelectionChanged();  // syncs tree → document selection state

private:
    QTreeWidgetItem* itemForBody(quint64 id) const;

    QTreeWidget* m_tree{nullptr};
    QLabel*      m_selectedLabel{nullptr};
    QListWidget* m_selectedList{nullptr};
    Document*    m_doc{nullptr};
    bool         m_updating{false};
};

} // namespace elcad
