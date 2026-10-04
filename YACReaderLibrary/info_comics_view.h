#ifndef INFOCOMICSVIEW_H
#define INFOCOMICSVIEW_H

#include "comics_view.h"
#include "themable.h"

#include <QQmlContext>

class QQuickView;
class QSettings;
class QSplitter;

class YACReaderComicsSelectionHelper;
class YACReaderComicInfoHelper;
class YACReaderTableView;

class InfoComicsView : public ComicsView, protected Themable
{
    Q_OBJECT
public:
    explicit InfoComicsView(QWidget *parent = nullptr);

protected:
    void applyTheme(const Theme &theme) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    ~InfoComicsView() override;
    void setToolBar(QToolBar *toolBar) override;
    void releaseToolBar() override;
    void saveViewConfig() override;
    void setModel(ComicModel *model) override;
    void setCurrentIndex(const QModelIndex &index) override;
    QModelIndex currentIndex() override;
    QItemSelectionModel *selectionModel() override;
    void scrollTo(const QModelIndex &mi, QAbstractItemView::ScrollHint hint) override;
    void toFullScreen() override;
    void toNormal() override;
    void updateConfig(QSettings *settings) override;
    void enableFilterMode(bool enabled) override;
    void selectIndex(int index) override;
    void updateCurrentComicView() override;
    void focusComicsNavigation(Qt::FocusReason reason) override;
    ContentViewState captureViewState() const override;
    void restoreViewState(const ContentViewState &state) override;

public slots:
    void setShowMarks(bool show) override;
    void selectAll() override;

protected slots:
    void setCurrentIndex(int index);

    bool canDropUrls(const QList<QUrl> &urls, Qt::DropAction action);
    bool canDropImage(const QList<QUrl> &urls);
    void droppedFiles(const QList<QUrl> &urls, Qt::DropAction action);
    void droppedImage(const QList<QUrl> &urls);

    void requestedContextMenu(const QPoint &point);

    void selectedItem(int index);

    void updateFlowFromTable(const QModelIndex &current);
    void saveTableHeadersStatus();
    void saveSplitterStatus();
    void requestedTableContextMenu(const QPoint &point);
    void requestedHeaderContextMenu(const QPoint &point);

protected:
    QToolBar *toolbar;
    QObject *flow;
    QObject *list;

    // The flow has its own QQuickView, so the table can be placed next to the info panel
    QQuickView *flowView;
    QWidget *flowContainer;
    QSplitter *flowSplitter;
    QSplitter *infoSplitter;
    YACReaderTableView *tableView;
    QSettings *settings;
    bool updatingFlowFromTable;
    bool useDefaultInfoSplitterSizes;

    YACReaderComicsSelectionHelper *selectionHelper;
    YACReaderComicInfoHelper *comicInfoHelper;
};

#endif // INFOCOMICSVIEW_H
