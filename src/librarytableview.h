#ifndef LIBRARYTABLEVIEW_H
#define LIBRARYTABLEVIEW_H

#include <QTableView>

class LibraryTableView : public QTableView
{
    Q_OBJECT

public:
    LibraryTableView(QWidget *parent = Q_NULLPTR);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // LIBRARYTABLEVIEW_H
