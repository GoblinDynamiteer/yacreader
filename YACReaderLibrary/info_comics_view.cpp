#include "info_comics_view.h"

#include "QsLog.h"
#include "comic.h"
#include "comic_files_manager.h"
#include "comic_model.h"
#include "comics_table_header.h"
#include "cover_utils.h"
#include "yacreader_comic_info_helper.h"
#include "yacreader_comics_selection_helper.h"
#include "yacreader_global.h"
#include "yacreader_global_gui.h"
#include "yacreader_table_view.h"

#include <QGuiApplication>
#include <QHeaderView>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickView>
#include <QSettings>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>
#include <QtMath>

InfoComicsView::InfoComicsView(QWidget *parent)
    : ComicsView(parent), toolbar(nullptr), flow(nullptr), list(nullptr), updatingFlowFromTable(false)
{
    settings = new QSettings(YACReader::getSettingsPath() + "/YACReaderLibrary.ini", QSettings::IniFormat, this);
    settings->beginGroup("libraryConfig");

    // container->setFocusPolicy(Qt::StrongFocus);

    QQmlContext *ctxt = view->rootContext();

    // The flow and the info panel are in different native windows, widgets can't be placed on
    // top of them. Both views share the QML engine, so they also share the context properties.
    flowView = new QQuickView(view->engine(), nullptr);
    flowView->setResizeMode(QQuickView::SizeRootObjectToView);
    connect(flowView, &QQuickView::statusChanged, this, [this](QQuickView::Status status) {
        if (status == QQuickView::Error)
            QLOG_ERROR() << flowView->errors();
    });
    flowContainer = QWidget::createWindowContainer(flowView);
    flowContainer->setFocusPolicy(Qt::StrongFocus);
    flowView->installEventFilter(this);

    tableView = new YACReaderTableView;
    tableView->verticalHeader()->hide();
    tableView->setFocusPolicy(Qt::StrongFocus);
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    infoSplitter = new QSplitter(Qt::Horizontal);
    infoSplitter->addWidget(container);
    infoSplitter->addWidget(tableView);
    // The info panel keeps its width when the window is resized
    infoSplitter->setStretchFactor(0, 0);
    infoSplitter->setStretchFactor(1, 1);
    useDefaultInfoSplitterSizes = !infoSplitter->restoreState(settings->value(INFO_COMICS_VIEW_SPLITTER_STATUS).toByteArray());

    // The covers are sized to the height of the flow
    flowContainer->setMinimumHeight(120);
    flowSplitter = new QSplitter(Qt::Vertical);
    flowSplitter->addWidget(flowContainer);
    flowSplitter->addWidget(infoSplitter);
    flowSplitter->setChildrenCollapsible(false);
    // The flow keeps its height when the window is resized
    flowSplitter->setStretchFactor(0, 0);
    flowSplitter->setStretchFactor(1, 1);

    // Apply theme colors
    initTheme(this);

    ctxt->setContextProperty("backgroundImage", QUrl());
    ctxt->setContextProperty("comicsList", new ComicModel());

    flowView->setSource(QUrl("qrc:/qml/InfoComicsFlowView.qml"));
    view->setSource(QUrl("qrc:/qml/InfoComicsView.qml"));

    if (!flowSplitter->restoreState(settings->value(INFO_COMICS_VIEW_FLOW_SPLITTER_STATUS).toByteArray())) {
        const int flowHeight = flowView->rootObject() != nullptr ? qCeil(flowView->rootObject()->implicitHeight()) : 267;
        flowSplitter->setSizes({ flowHeight, 1 });
    }

    selectionHelper = new YACReaderComicsSelectionHelper(this);
    comicInfoHelper = new YACReaderComicInfoHelper(this);

    connect(tableView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &index) { selectedItem(index.row()); });
    connect(tableView, &YACReaderTableView::comicRated, this, &ComicsView::comicRated);
    connect(tableView, &YACReaderTableView::customCoverDropped, this, [this](const QString &imagePath, const QModelIndex &index) {
        emit customComicCoverRequested(index.data(ComicModel::IdRole).toULongLong(), imagePath);
    });
    connect(tableView, &QWidget::customContextMenuRequested, this, &InfoComicsView::requestedTableContextMenu);
    connect(tableView->horizontalHeader(), &QWidget::customContextMenuRequested, this, &InfoComicsView::requestedHeaderContextMenu);
    connect(tableView->horizontalHeader(), &QHeaderView::sectionMoved, this, &InfoComicsView::saveTableHeadersStatus);
    connect(tableView->horizontalHeader(), &QHeaderView::sectionResized, this, &InfoComicsView::saveTableHeadersStatus);
    connect(infoSplitter, &QSplitter::splitterMoved, this, [this] {
        useDefaultInfoSplitterSizes = false;
        saveSplitterStatus();
    });
    connect(flowSplitter, &QSplitter::splitterMoved, this, &InfoComicsView::saveSplitterStatus);

    auto l = new QVBoxLayout;
    l->addWidget(flowSplitter);
    this->setLayout(l);

    setContentsMargins(0, 0, 0, 0);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    setShowMarks(true);

    QLOG_TRACE() << "InfoComicsView";
}

InfoComicsView::~InfoComicsView()
{
    // The flow view uses the engine owned by view
    delete flowView;
    delete view;
}

bool InfoComicsView::eventFilter(QObject *watched, QEvent *event)
{
    // Same as ComicsView::eventFilter, for the flow
    if (watched == flowView && event->type() == QEvent::MouseButtonPress) {
        if (!flowContainer->hasFocus())
            flowContainer->setFocus(Qt::MouseFocusReason);
        else if (QGuiApplication::focusWindow() != flowView)
            flowView->requestActivate();
    }

    return ComicsView::eventFilter(watched, event);
}

void InfoComicsView::resizeEvent(QResizeEvent *event)
{
    ComicsView::resizeEvent(event);

    // Until the splitter is moved, the info panel gets 40% of the width and the table the rest
    if (useDefaultInfoSplitterSizes) {
        const int infoWidth = width() * 2 / 5;
        infoSplitter->setSizes({ infoWidth, width() - infoWidth });
    }
}

void InfoComicsView::setToolBar(QToolBar *toolBar)
{
    static_cast<QVBoxLayout *>(this->layout())->addWidget(toolBar);
    toolbar = toolBar;
}

void InfoComicsView::releaseToolBar()
{
}

void InfoComicsView::saveViewConfig()
{
    saveTableHeadersStatus();
    saveSplitterStatus();
}

void InfoComicsView::setModel(ComicModel *model)
{
    if (model == NULL)
        return;

    selectionHelper->setModel(model);
    comicInfoHelper->setModel(model);

    ComicsView::setModel(model);

    tableView->horizontalHeader()->blockSignals(true);

    tableView->setModel(model);
    // The table shares the selection with the flow, setModel() gives it a new selection model
    auto tableSelectionModel = tableView->selectionModel();
    tableView->setSelectionModel(selectionHelper->selectionModel());
    delete tableSelectionModel;
    connect(selectionHelper->selectionModel(), &QItemSelectionModel::currentChanged, this, &InfoComicsView::updateFlowFromTable);

    auto headersState = settings->value(INFO_COMICS_VIEW_HEADERS).toByteArray();
    if (headersState.isEmpty()) // start with the columns of the classic view
        headersState = settings->value(COMICS_VIEW_HEADERS).toByteArray();
    YACReader::setupComicsTableHeader(tableView, headersState);

    tableView->horizontalHeader()->blockSignals(false);

    QQmlContext *ctxt = view->rootContext();

    /*if(_selectionModel != NULL)
        delete _selectionModel;

    _selectionModel = new QItemSelectionModel(model);*/

    ctxt->setContextProperty("comicsList", model);
    if (model->rowCount() > 0)
        ctxt->setContextProperty("backgroundImage", this->model->data(this->model->index(0, 0), ComicModel::CoverPathRole));
    else
        ctxt->setContextProperty("backgroundImage", QUrl());

    ctxt->setContextProperty("comicsSelection", selectionHelper->selectionModel());
    ctxt->setContextProperty("contextMenuHelper", this);
    ctxt->setContextProperty("currentIndexHelper", this);
    ctxt->setContextProperty("comicInfoHelper", comicInfoHelper);
    /*ctxt->setContextProperty("comicsSelectionHelper", this);
    ctxt->setContextProperty("dragManager", this);*/
    ctxt->setContextProperty("dropManager", this);

    if (model->rowCount() > 0) {
        setCurrentIndex(model->index(0, 0));
        updateInfoForIndex(0);
    }

    if (flow != nullptr) {
        flow->disconnect();
    }

    if (list != nullptr) {
        list->disconnect();
    }

    if (auto *rootObject = flowView->rootObject()) {
        flow = rootObject->findChild<QQuickItem *>("flow", Qt::FindChildrenRecursively);
        list = rootObject->findChild<QQuickItem *>("list", Qt::FindChildrenRecursively);
    }

    if (flow) {
        // QML signals only work with old style signal slot syntax
        connect(flow, SIGNAL(currentCoverChanged(int)), this, SLOT(updateInfoForIndex(int))); // clazy:exclude=old-style-connect
        connect(flow, SIGNAL(currentCoverChanged(int)), this, SLOT(setCurrentIndex(int))); // clazy:exclude=old-style-connect
    }
}

void InfoComicsView::setCurrentIndex(const QModelIndex &index)
{
    if (list != nullptr) {
        QQmlProperty(list, "currentIndex").write(index.row());
    }

    setCurrentIndex(index.row());
}

void InfoComicsView::setCurrentIndex(int index)
{
    // The table already selected the comics (there can be more than one)
    if (updatingFlowFromTable)
        return;

    selectionHelper->clear();
    selectionHelper->selectIndex(index);

    if (model != nullptr && index >= 0 && index < model->rowCount()) {
        const auto modelIndex = model->index(index, 0);
        selectionHelper->selectionModel()->setCurrentIndex(modelIndex, QItemSelectionModel::NoUpdate);
        tableView->scrollTo(modelIndex);
    }
}

QModelIndex InfoComicsView::currentIndex()
{
    return selectionHelper->currentIndex();
}

QItemSelectionModel *InfoComicsView::selectionModel()
{
    return selectionHelper->selectionModel();
}

void InfoComicsView::scrollTo(const QModelIndex &mi, QAbstractItemView::ScrollHint hint)
{
    tableView->scrollTo(mi, hint);
}

ContentViewState InfoComicsView::captureViewState() const
{
    ContentViewState state;
    const auto index = selectionHelper->currentIndex();
    if (index.isValid()) {
        state.topItem.kind = ContentItemRef::Comic;
        state.topItem.id = index.data(ComicModel::IdRole).toULongLong();
        state.fallbackComicRow = index.row();
        state.currentItem = state.topItem;
    }
    return state;
}

void InfoComicsView::restoreViewState(const ContentViewState &state)
{
    if (!model || model->rowCount() == 0)
        return;

    auto index = state.currentItem.kind == ContentItemRef::Comic ? model->getIndexFromId(state.currentItem.id) : QModelIndex();
    if (!index.isValid() && state.fallbackComicRow >= 0)
        index = model->index(qBound(0, state.fallbackComicRow, model->rowCount() - 1), 0);
    if (!index.isValid())
        return;

    selectionHelper->clear();
    selectionHelper->selectIndex(index.row());
    if (list)
        QMetaObject::invokeMethod(list, "restoreCurrentIndex", Q_ARG(QVariant, index.row()));
    selectionHelper->selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    tableView->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void InfoComicsView::toFullScreen()
{
    toolbar->hide();
}

void InfoComicsView::toNormal()
{
    toolbar->show();
}

void InfoComicsView::updateConfig(QSettings *settings)
{
    Q_UNUSED(settings);
}

void InfoComicsView::enableFilterMode(bool enabled)
{
    Q_UNUSED(enabled);
}

void InfoComicsView::selectIndex(int index)
{
    selectionHelper->selectIndex(index);
}

void InfoComicsView::updateCurrentComicView()
{
}

void InfoComicsView::focusComicsNavigation(Qt::FocusReason reason)
{
    flowContainer->setFocus(reason);
}

void InfoComicsView::setShowMarks(bool show)
{
    QQmlContext *ctxt = view->rootContext();
    ctxt->setContextProperty("show_marks", show);
}

void InfoComicsView::selectAll()
{
    selectionHelper->selectAll();
}

bool InfoComicsView::canDropUrls(const QList<QUrl> &urls, Qt::DropAction action)
{
    if (action == Qt::CopyAction) {
        QString currentPath;
        for (const auto &url : urls) {
            // comics or folders are accepted, folders' content is validate in dropEvent (avoid any lag before droping)
            currentPath = url.toLocalFile();
            if (Comic::fileIsComic(currentPath) || QFileInfo(currentPath).isDir())
                return true;
        }
    }
    return false;
}

bool InfoComicsView::canDropImage(const QList<QUrl> &urls)
{
    return !YACReader::droppedImagePath(urls).isEmpty() && currentIndex().isValid();
}

void InfoComicsView::droppedFiles(const QList<QUrl> &urls, Qt::DropAction action)
{
    bool validAction = action == Qt::CopyAction; // TODO add move

    if (validAction) {
        QList<QPair<QString, QString>> droppedFiles = ComicFilesManager::getDroppedFiles(urls);
        emit copyComicsToCurrentFolder(droppedFiles);
    }
}

void InfoComicsView::droppedImage(const QList<QUrl> &urls)
{
    const auto imagePath = YACReader::droppedImagePath(urls);
    const auto index = currentIndex();
    if (!imagePath.isEmpty() && index.isValid())
        emit customComicCoverRequested(index.data(ComicModel::IdRole).toULongLong(), imagePath);
}

void InfoComicsView::requestedContextMenu(const QPoint &point)
{
    emit customContextMenuViewRequested(point);
}

void InfoComicsView::selectedItem(int index)
{
    emit selected(index);
}

void InfoComicsView::updateFlowFromTable(const QModelIndex &current)
{
    if (!current.isValid() || list == nullptr)
        return;

    // The flow notifies the change with currentCoverChanged, which also updates the info panel
    updatingFlowFromTable = true;
    QQmlProperty(list, "currentIndex").write(current.row());
    updatingFlowFromTable = false;
}

void InfoComicsView::saveTableHeadersStatus()
{
    if (model == nullptr || model->rowCount() == 0)
        return;

    settings->setValue(INFO_COMICS_VIEW_HEADERS, tableView->horizontalHeader()->saveState());
}

void InfoComicsView::saveSplitterStatus()
{
    settings->setValue(INFO_COMICS_VIEW_FLOW_SPLITTER_STATUS, flowSplitter->saveState());

    // Until it is moved, the info splitter keeps its default proportions
    if (!useDefaultInfoSplitterSizes)
        settings->setValue(INFO_COMICS_VIEW_SPLITTER_STATUS, infoSplitter->saveState());
}

void InfoComicsView::requestedTableContextMenu(const QPoint &point)
{
    emit customContextMenuItemRequested(tableView->mapTo(this, point));
}

void InfoComicsView::requestedHeaderContextMenu(const QPoint &point)
{
    YACReader::execComicsTableHeaderMenu(tableView, tableView->horizontalHeader()->mapToGlobal(point), [this] { saveTableHeadersStatus(); });
}

void InfoComicsView::applyTheme(const Theme &theme)
{
    QQmlContext *ctxt = view->rootContext();
    const auto &giv = theme.gridAndInfoView;

    flowSplitter->setStyleSheet(theme.contentSplitter.verticalSplitterQSS);
    infoSplitter->setStyleSheet(theme.contentSplitter.horizontalSplitterQSS);

    // Info panel colors
    // Cache-bust the SVG file URLs so QML's image cache doesn't serve stale
    // files when the theme is updated (the same file path is rewritten each time).
    const QString bust = QString::number(QDateTime::currentMSecsSinceEpoch());
    auto svgUrl = [&bust](const QString &path) {
        QUrl url = QUrl::fromLocalFile(path);
        url.setQuery(bust);
        return url;
    };
    ctxt->setContextProperty("infoBackgroundColor", giv.infoBackgroundColor);
    ctxt->setContextProperty("topShadow", svgUrl(giv.topShadow));
    ctxt->setContextProperty("infoShadow", svgUrl(giv.infoShadow));
    ctxt->setContextProperty("infoIndicator", svgUrl(giv.infoIndicator));
    ctxt->setContextProperty("infoMetadataTextColor", giv.infoMetadataTextColor);
    ctxt->setContextProperty("infoTextColor", giv.infoTextColor);

    // Rating and favorite colors
    ctxt->setContextProperty("ratingUnselectedColor", giv.ratingUnselectedColor);
    ctxt->setContextProperty("ratingSelectedColor", giv.ratingSelectedColor);
    ctxt->setContextProperty("favUncheckedColor", giv.favUncheckedColor);
    ctxt->setContextProperty("favCheckedColor", giv.favCheckedColor);
    ctxt->setContextProperty("readTickUncheckedColor", giv.readTickUncheckedColor);
    ctxt->setContextProperty("readTickCheckedColor", giv.readTickCheckedColor);

    ctxt->setContextProperty("showDropShadow", QVariant(giv.showDropShadow));
    ctxt->setContextProperty("backgroundBlurOverlayColor", giv.backgroundBlurOverlayColor);

    // Info panel scrollbar, comic cover border, links
    ctxt->setContextProperty("infoScrollbarColor", giv.infoScrollbarColor);
    ctxt->setContextProperty("comicCoverBorderColor", giv.comicCoverBorderColor);
    ctxt->setContextProperty("themeLinkColor", giv.linkColor);
    ctxt->setContextProperty("themeLinkColorStr", giv.linkColor.name());
}
