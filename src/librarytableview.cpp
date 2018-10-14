#include "librarytableview.h"

#include <QKeyEvent>

LibraryTableView::LibraryTableView(QWidget *parent) : QTableView(parent)
{
}

void LibraryTableView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Tab &&
        (event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::NoModifier)) {
        event->ignore();
    } else {
        QTableView::keyPressEvent(event);
    }
}
