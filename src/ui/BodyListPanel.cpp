#include "ui/BodyListPanel.h"
#include "document/Document.h"
#include "document/Body.h"
#include "sketch/Sketch.h"
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

    // Selected items list below bodies
    m_selectedLabel = new QLabel("Selection", this);
    m_selectedLabel->setStyleSheet("font-weight: bold; padding-top: 6px;");
    layout->addWidget(m_selectedLabel);

    m_selectedList = new QListWidget(this);
    m_selectedList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_selectedList->setFixedHeight(120);
    layout->addWidget(m_selectedList);

    // Sketch list section
    m_sketchLabel = new QLabel("Sketches", this);
    m_sketchLabel->setStyleSheet("font-weight: bold; padding-top: 6px;");
    layout->addWidget(m_sketchLabel);

    m_sketchList = new QListWidget(this);
    m_sketchList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_sketchList);

    connect(m_tree, &QTreeWidget::itemChanged,
            this,   &BodyListPanel::onItemChanged);
    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this,   &BodyListPanel::onSelectionChanged);
    connect(m_sketchList, &QListWidget::itemChanged,
            this,         &BodyListPanel::onSketchItemChanged);
    // No action on selected-list clicks for now
}

void BodyListPanel::setDocument(Document* doc)
{
    if (m_doc) {
        disconnect(m_doc, nullptr, this, nullptr);
    }
    m_doc = doc;
    m_tree->clear();
    m_sketchList->clear();

    if (!m_doc) return;

    // Populate with existing bodies
    for (auto& b : m_doc->bodies())
        onBodyAdded(b.get());

    // Populate with existing sketches
    for (auto& s : m_doc->sketches())
        onSketchAdded(s.get());

    connect(m_doc, &Document::bodyAdded,   this, &BodyListPanel::onBodyAdded);
    connect(m_doc, &Document::bodyRemoved, this, &BodyListPanel::onBodyRemoved);
    connect(m_doc, &Document::selectionChanged, this, &BodyListPanel::onDocumentSelectionChanged);
    connect(m_doc, &Document::sketchAdded,             this, &BodyListPanel::onSketchAdded);
    connect(m_doc, &Document::sketchRemoved,           this, &BodyListPanel::onSketchRemoved);
    connect(m_doc, &Document::sketchVisibilityChanged, this, &BodyListPanel::onSketchVisibilityChanged);
    connect(m_doc, &Document::activeSketchChanged,     this, &BodyListPanel::onActiveSketchChanged);
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

    std::vector<Document::SelectedItem> items;
    items.reserve(m_tree->selectedItems().size());
    for (auto* item : m_tree->selectedItems()) {
        quint64 id = item->data(0, Qt::UserRole).toULongLong();
        if (m_doc->bodyById(id)) {
            Document::SelectedItem selectedItem;
            selectedItem.type = Document::SelectedItem::Type::Body;
            selectedItem.bodyId = id;
            selectedItem.index = -1;
            items.push_back(selectedItem);
        }
    }
    m_doc->setSelection(items);
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

    // Sync tree items to document selectionItems (supports mixed-type selection)
    m_tree->clearSelection();
    m_selectedList->clear();

    auto sel = m_doc->selectionItems();
    for (const auto& s : sel) {
        if (s.type == Document::SelectedItem::Type::Body) {
            if (auto* item = itemForBody(s.bodyId))
                item->setSelected(true);
            if (Body* b = m_doc->bodyById(s.bodyId))
                m_selectedList->addItem(QString("Body: %1 (id=%2)").arg(b->name()).arg(s.bodyId));
        } else if (s.type == Document::SelectedItem::Type::Face) {
            if (Body* b = m_doc->bodyById(s.bodyId))
                m_selectedList->addItem(QString("Face: tri %1 on Body '%2' (id=%3)")
                                        .arg(s.index).arg(b->name()).arg(s.bodyId));
        } else if (s.type == Document::SelectedItem::Type::Edge) {
            int tri = s.index / 3;
            int local = s.index % 3;
            if (Body* b = m_doc->bodyById(s.bodyId))
                m_selectedList->addItem(QString("Edge: tri %1 local %2 on Body '%3' (id=%4)")
                                        .arg(tri).arg(local).arg(b->name()).arg(s.bodyId));
        } else if (s.type == Document::SelectedItem::Type::Vertex) {
            int tri = s.index / 3;
            int local = s.index % 3;
            if (Body* b = m_doc->bodyById(s.bodyId))
                m_selectedList->addItem(QString("Vertex: tri %1 local %2 on Body '%3' (id=%4)")
                                        .arg(tri).arg(local).arg(b->name()).arg(s.bodyId));
        }
    }

    m_updating = false;
}

// ── Sketch list slots ──────────────────────────────────────────────────────────

void BodyListPanel::onSketchAdded(Sketch* sketch)
{
    if (!sketch) return;
    m_updating = true;
    auto* item = new QListWidgetItem(sketch->name(), m_sketchList);
    item->setData(Qt::UserRole, static_cast<quint64>(sketch->id()));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(sketch->visible() ? Qt::Checked : Qt::Unchecked);
    m_sketchList->addItem(item);
    m_updating = false;
}

void BodyListPanel::onSketchRemoved(quint64 id)
{
    if (auto* item = itemForSketch(id))
        delete item;
}

void BodyListPanel::onSketchVisibilityChanged(Sketch* sketch)
{
    if (!sketch) return;
    m_updating = true;
    if (auto* item = itemForSketch(sketch->id()))
        item->setCheckState(sketch->visible() ? Qt::Checked : Qt::Unchecked);
    m_updating = false;
}

void BodyListPanel::onActiveSketchChanged(Sketch* sketch)
{
    // When a sketch is reactivated (being edited), italicise its item so the user
    // can see which one is active. When editing ends (sketch == nullptr), restore all.
    m_updating = true;
    for (int i = 0; i < m_sketchList->count(); ++i) {
        auto* item = m_sketchList->item(i);
        QFont f = item->font();
        f.setItalic(sketch && item->data(Qt::UserRole).toULongLong() == sketch->id());
        item->setFont(f);
    }
    m_updating = false;
}

void BodyListPanel::onSketchItemChanged(QListWidgetItem* item)
{
    if (m_updating || !m_doc) return;
    quint64 id = item->data(Qt::UserRole).toULongLong();
    m_doc->setSketchVisible(id, item->checkState() == Qt::Checked);
}

QListWidgetItem* BodyListPanel::itemForSketch(quint64 id) const
{
    for (int i = 0; i < m_sketchList->count(); ++i) {
        auto* item = m_sketchList->item(i);
        if (item->data(Qt::UserRole).toULongLong() == id)
            return item;
    }
    return nullptr;
}

} // namespace elcad

