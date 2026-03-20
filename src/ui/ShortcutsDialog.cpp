#include "ui/ShortcutsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QLabel>

namespace elcad {

ShortcutsDialog::ShortcutsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Keyboard Shortcuts");
    setMinimumWidth(420);

    struct Row { const char* key; const char* action; };
    static constexpr Row rows[] = {
        // General
        { "Ctrl+N",       "New document"             },
        { "Ctrl+Z",       "Undo"                     },
        { "Ctrl+Y",       "Redo"                     },
        { "Ctrl+I",       "Import STEP"              },
        { "Ctrl+S",       "Export STEP"              },
        // View
        { "G",            "Toggle grid"              },
        { "Shift+G",      "Toggle snap"              },
        { "F",            "Fit all"                  },
        { "P",            "Toggle perspective/ortho" },
        { "1 / Num 1",    "Front view"               },
        { "3 / Num 3",    "Right view"               },
        { "7 / Num 7",    "Top view"                 },
        { "0 / Num 0",    "Isometric view"           },
        // Gizmo
        { "T",            "Gizmo: Translate"         },
        { "R",            "Gizmo: Rotate"            },
        { "S",            "Gizmo: Scale"             },
        // Sketch / Selection
        { "Esc",          "Exit sketch / Clear selection" },
    };

    auto* layout = new QVBoxLayout(this);

    auto* table = new QTableWidget(static_cast<int>(std::size(rows)), 2, this);
    table->setHorizontalHeaderLabels({ "Shortcut", "Action" });
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setAlternatingRowColors(true);

    for (int i = 0; i < static_cast<int>(std::size(rows)); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(rows[i].key));
        table->setItem(i, 1, new QTableWidgetItem(rows[i].action));
    }

    layout->addWidget(table);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(btns);
}

} // namespace elcad
