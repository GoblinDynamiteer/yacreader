#ifndef COMICS_TABLE_HEADER_H
#define COMICS_TABLE_HEADER_H

#include <QByteArray>
#include <QPoint>

#include <functional>

class QTableView;

namespace YACReader {

// Restores the columns of a table showing a ComicModel from a QHeaderView::saveState() value,
// or shows the default columns if there is nothing to restore.
void setupComicsTableHeader(QTableView *tableView, const QByteArray &savedState);

// Shows a menu to choose the visible columns of a table showing a ComicModel.
// columnsChanged is called every time a column is shown or hidden.
void execComicsTableHeaderMenu(QTableView *tableView, const QPoint &globalPos, const std::function<void()> &columnsChanged);

}

#endif // COMICS_TABLE_HEADER_H
