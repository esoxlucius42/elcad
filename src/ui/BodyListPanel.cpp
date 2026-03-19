#include "ui/BodyListPanel.h"
#include "document/Document.h"
#include "document/Body.h"
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QHeaderView>

namespace elcad {

BodyListPanel::BodyListPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabel("Scene Bodies");
    m_tree->setRootIsDecorated(false);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->header()->setStretchLastSection(true);
    layout->addWidget(m_tree);

    connect(m_tree, &QTreeWidget::itemChanged,
            this,   &BodyListPanel::onItemChanged);
    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this,   &BodyListPanel::onSelectionChanged);
}

void BodyListPanel::setDocument(Document* doc)
{
    if (m_doc) {
        disconnect(m_doc, nullptr, this, nullptr);
    }
    m_doc = doc;
    m_tree->clear();

    if (!m_doc) return;

    // Populate with existing bodies
    for (auto& b : m_doc->bodies())
        onBodyAdded(b.get());

    connect(m_doc, &Document::bodyAdded,   this, &BodyListPanel::onBodyAdded);
    connect(m_doc, &Document::bodyRemoved, this, &BodyListPanel::onBodyRemoved);
    connect(m_doc, &Document::selectionChanged, this, &BodyListPanel::onDocumentSelectionChanged);
}

void BodyListPanel::onBodyAdded(Body* body)
{
    m_updating = true;
    auto* item = new QTreeWidgetItem(m_tree);
    item->setText(0, body->name());
    item->setData(0, Qt::UserRole, static_cast<quint64>(body->id()));
    item->setCheckState(0, body->visible() ? Qt::Checked : Qt::Unchecked);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    // Colour swatch
    QPixmap px(12, 12);
    px.fill(body->color());
    item->setIcon(0, QIcon(px));

    m_tree->addTopLevelItem(item);
    m_updating = false;
}

void BodyListPanel::onBodyRemoved(quint64 id)
{
    auto* item = itemForBody(id);
    if (item) delete item;
}

void BodyListPanel::onItemChanged(QTreeWidgetItem* item, int /*column*/)
{
    if (m_updating || !m_doc) return;
    quint64 id = item->data(0, Qt::UserRole).toULongLong();
    Body* body = m_doc->bodyById(id);
    if (!body) return;

    body->setVisible(item->checkState(0) == Qt::Checked);
    body->setName(item->text(0));
    emit m_doc->bodyChanged(body);
}

void BodyListPanel::onSelectionChanged()
{
    if (m_updating || !m_doc) return;
    m_updating = true;

    m_doc->clearSelection();
    for (auto* item : m_tree->selectedItems()) {
        quint64 id = item->data(0, Qt::UserRole).toULongLong();
        if (Body* b = m_doc->bodyById(id))
            b->setSelected(true);
    }
    emit m_doc->selectionChanged();
    m_updating = false;
}

QTreeWidgetItem* BodyListPanel::itemForBody(quint64 id) const
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* item = m_tree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toULongLong() == id)
            return item;
    }
    return nullptr;
}

void BodyListPanel::onDocumentSelectionChanged()
{
    if (m_updating || !m_doc) return;
    m_updating = true;

    // Sync tree items to body selected state (set externally via viewport pick)
    m_tree->clearSelection();
    for (auto& bodyPtr : m_doc->bodies()) {
        if (bodyPtr->selected()) {
            if (auto* item = itemForBody(bodyPtr->id()))
                item->setSelected(true);
        }
    }

    m_updating = false;
}

} // namespace elcad
