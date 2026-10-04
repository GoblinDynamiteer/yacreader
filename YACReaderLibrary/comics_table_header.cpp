#include "comics_table_header.h"

#include "comic_model.h"

#include <QHeaderView>
#include <QMenu>
#include <QTableView>

void YACReader::setupComicsTableHeader(QTableView *tableView, const QByteArray &savedState)
{
    auto header = tableView->horizontalHeader();

    bool loadDefaults = true;
    if (!savedState.isEmpty()) {
        try {
            loadDefaults = !header->restoreState(savedState);
        } catch (...) {
        }
    }

    if (loadDefaults) {
        // default columns and order
        for (int i = 0; i < header->count(); i++)
            header->hideSection(i);

        header->showSection(ComicModel::Number);
        header->showSection(ComicModel::Title);
        header->showSection(ComicModel::FileName);
        header->showSection(ComicModel::NumPages);
        header->showSection(ComicModel::Size);
        header->showSection(ComicModel::ReadColumn);
        header->showSection(ComicModel::CurrentPage);
        header->showSection(ComicModel::PublicationDate);
        header->showSection(ComicModel::Rating);

        header->moveSection(ComicModel::CurrentPage, 3);
        header->moveSection(ComicModel::Rating, 12);
        header->moveSection(ComicModel::Size, 5);
    }

    // make sure that columns without title are hidden
    for (int i = 0; i < header->count(); i++) {
        auto title = tableView->model()->headerData(i, Qt::Horizontal).toString();
        if (title.isEmpty()) {
            header->hideSection(i);
        }
    }

    header->setDefaultAlignment(Qt::AlignLeft);
    header->setSectionsMovable(true);
    header->setStretchLastSection(true);

    for (int i = 0; i < header->count() - 1; i++) {
        if (!header->isSectionHidden(i)) {
            tableView->resizeColumnToContents(i);
        }
    }
}

void YACReader::execComicsTableHeaderMenu(QTableView *tableView, const QPoint &globalPos, const std::function<void()> &columnsChanged)
{
    QMenu menu;

    for (int i = 0; i < tableView->model()->columnCount(); ++i) {
        auto title = tableView->model()->headerData(i, Qt::Horizontal).toString();

        if (!title.isEmpty()) {
            auto action = menu.addAction(title);
            action->setCheckable(true);
            action->setChecked(!tableView->isColumnHidden(i));
            QObject::connect(action, &QAction::toggled, tableView, [tableView, i, columnsChanged](bool checked) {
                tableView->horizontalHeader()->setSectionHidden(i, !checked);
                columnsChanged();
            });
        }
    }

    menu.exec(globalPos);
}
