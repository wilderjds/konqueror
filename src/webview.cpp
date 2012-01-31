/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "webview.h"
#include "webpage.h"
#include "kwebkitpart.h"
#include "settings/webkitsettings.h"

#include <kio/global.h>
#include <KDE/KAboutData>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KConfigGroup>
#include <KDE/KMimeType>
#include <KDE/KService>
#include <KDE/KUriFilter>
#include <KDE/KStandardDirs>
#include <KDE/KActionMenu>
#include <KDE/KIO/AccessManager>
#include <KDE/KStringHandler>

#include <QtCore/QMimeData>
#include <QtGui/QDropEvent>
#include <QtNetwork/QHttpRequestHeader>
#include <QtNetwork/QNetworkRequest>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebHitTestResult>
#include <QtWebKit/QWebInspector>

#define QL1S(x)   QLatin1String(x)
#define ALTERNATE_DEFAULT_PROVIDER    QL1S("google")
#define ALTERNATE_SEARCH_PROVIDERS    QStringList() << QL1S("google") << QL1S("wikipedia") << QL1S("webster") << QL1S("dmoz")

WebView::WebView(KWebKitPart* part, QWidget* parent)
        :KWebView(parent, false),
         m_actionCollection(new KActionCollection(this)),
         m_part(QWeakPointer<KWebKitPart>(part)),
         m_webInspector(0)
{
    setAcceptDrops(true);

    // Create the custom page...
    setPage(new WebPage(part, this));
}

WebView::~WebView()
{
    //kDebug();
}

void WebView::loadUrl(const KUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& bargs)
{
    page()->setProperty("NavigationTypeUrlEntered", true);

    if (args.reload() && url == this->url()) {
      reload();
      return;
    }

    QNetworkRequest request(url);
    if (args.reload()) {
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    }

    if (bargs.postData.isEmpty()) {
        KWebView::load(request);
    } else {
        KWebView::load(request, QNetworkAccessManager::PostOperation, bargs.postData);
    }
}

QWebHitTestResult WebView::contextMenuResult() const
{
    return m_result;
}

static bool isMultimediaElement(const QWebElement& element)
{
    if (element.tagName().compare(QL1S("video"), Qt::CaseInsensitive) == 0)
        return true;

    if (element.tagName().compare(QL1S("audio"), Qt::CaseInsensitive) == 0)
        return true;

    return false;
}

static void extractMimeTypeFor(const KUrl& url, QString& mimeType)
{
    const QString fname(url.fileName(KUrl::ObeyTrailingSlash));
 
    if (fname.isEmpty() || url.hasRef() || url.hasQuery())
        return;
 
    KMimeType::Ptr pmt = KMimeType::findByPath(fname, 0, true);

    // Further check for mime types guessed from the extension which,
    // on a web page, are more likely to be a script delivering content
    // of undecidable type. If the mime type from the extension is one
    // of these, don't use it.  Retain the original type 'text/html'.
    if (pmt->name() == KMimeType::defaultMimeType() ||
        pmt->is(QL1S("application/x-perl")) ||
        pmt->is(QL1S("application/x-perl-module")) ||
        pmt->is(QL1S("application/x-php")) ||
        pmt->is(QL1S("application/x-python-bytecode")) ||
        pmt->is(QL1S("application/x-python")) ||
        pmt->is(QL1S("application/x-shellscript")))
        return;

    mimeType = pmt->name();
}

void WebView::dropEvent(QDropEvent* ev)
{
#if (QTWEBKIT_VERSION < QTWEBKIT_VERSION_CHECK(2, 2, 0))
    // TODO: Workaround that should be removed one the bug in QtWebKit is fixed.
    // See https://bugs.webkit.org/show_bug.cgi?id=53320.
    if (ev) {
        QWebHitTestResult hitResult = page()->currentFrame()->hitTestContent(ev->pos());
        if (hitResult.isNull() || hitResult.element().tagName() != QL1S("input")) {
            const QMimeData* mimeData = ev ? ev->mimeData() : 0;
            if (mimeData && mimeData->hasUrls()) {
                KUrl url (mimeData->urls().first());
                emit m_part.data()->browserExtension()->openUrlRequest(url);
                ev->accept();
                return;
            }
        }
    }
#else
    QWebView::dropEvent(ev);
#endif    
}

void WebView::contextMenuEvent(QContextMenuEvent* e)
{
    m_result = page()->mainFrame()->hitTestContent(e->pos());
    if (m_result.isContentEditable()) {
        KWebView::contextMenuEvent(e); // TODO: better KDE integration if possible
        return;
    }

    // Clear the previous collection entries first...
    m_actionCollection->clear();

    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems;
    KParts::BrowserExtension::ActionGroupMap mapAction;
    QString mimeType (QL1S("text/html"));

    KUrl emitUrl;
    if (isMultimediaElement(m_result.element())) {
        multimediaActionPopupMenu(mapAction);
    } else if (!m_result.linkUrl().isValid()) {
        if (m_result.imageUrl().isValid()) {
            emitUrl = m_result.imageUrl();
            extractMimeTypeFor(emitUrl, mimeType);
        } else {
            flags |= KParts::BrowserExtension::ShowBookmark;
            flags |= KParts::BrowserExtension::ShowReload;
            emitUrl = m_part.data()->url();

            if (m_result.isContentSelected()) {
                flags |= KParts::BrowserExtension::ShowTextSelectionItems;
                selectActionPopupMenu(mapAction);
            } else {
                flags |= KParts::BrowserExtension::ShowNavigationItems;
            }
        }
        partActionPopupMenu(mapAction);
    } else {
        flags |= KParts::BrowserExtension::ShowBookmark;
        flags |= KParts::BrowserExtension::ShowReload;
        flags |= KParts::BrowserExtension::IsLink;
        emitUrl = m_result.linkUrl();
        linkActionPopupMenu(mapAction);
        if (emitUrl.isLocalFile())
            mimeType = KMimeType::findByUrl(emitUrl, 0, true, false)->name();
        else
            extractMimeTypeFor(emitUrl, mimeType);
        partActionPopupMenu(mapAction);
    }


    KParts::OpenUrlArguments args;
    KParts::BrowserArguments bargs;
    args.setMimeType(mimeType);
    emit m_part.data()->browserExtension()->popupMenu(e->globalPos(), emitUrl, 0, args, bargs, flags, mapAction);
}

void WebView::partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& partGroupMap)
{
    QList<QAction*> partActions;

    if (m_result.imageUrl().isValid()) {
        KAction *action;
        action = new KAction(i18n("Save Image As..."), this);
        m_actionCollection->addAction(QL1S("saveimageas"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotSaveImageAs()));
        partActions.append(action);

        action = new KAction(i18n("Send Image..."), this);
        m_actionCollection->addAction(QL1S("sendimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotSendImage()));
        partActions.append(action);

        action = new KAction(i18n("Copy Image URL"), this);
        m_actionCollection->addAction(QL1S("copyimageurl"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotCopyImageURL()));
        partActions.append(action);

        action = new KAction(i18n("Copy Image"), this);
        m_actionCollection->addAction(QL1S("copyimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotCopyImage()));
        action->setEnabled(!m_result.pixmap().isNull());
        partActions.append(action);

        action = new KAction(i18n("View Image (%1)", KUrl(m_result.imageUrl()).fileName()), this);
        m_actionCollection->addAction(QL1S("viewimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotViewImage()));
        partActions.append(action);

        if (WebKitSettings::self()->isAdFilterEnabled()) {
            action = new KAction(i18n("Block Image..."), this);
            m_actionCollection->addAction(QL1S("blockimage"), action);
            connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotBlockImage()));
            partActions.append(action);

            if (!m_result.imageUrl().host().isEmpty() &&
                !m_result.imageUrl().scheme().isEmpty())
            {
                action = new KAction( i18n( "Block Images From %1" , m_result.imageUrl().host()), this );
                m_actionCollection->addAction(QL1S("blockhost"), action);
                connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotBlockHost()));
                partActions.append(action);
            }
        }
    } else if (m_result.frame()->parentFrame() && !m_result.isContentSelected()) {
        KActionMenu * menu = new KActionMenu(i18nc("@title:menu HTML frame/iframe", "Frame"), this);

        KAction* action = new KAction(KIcon("window-new"), i18n("Open in New &Window"), this);
        m_actionCollection->addAction(QL1S("frameinwindow"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotFrameInWindow()));
        menu->addAction(action);

        action = new KAction(i18n("Open in &This Window"), this);
        m_actionCollection->addAction(QL1S("frameintop"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotFrameInTop()));
        menu->addAction(action);

        action = new KAction(KIcon("tab-new"), i18n("Open in &New Tab"), this);
        m_actionCollection->addAction(QL1S("frameintab"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotFrameInTab()));
        menu->addAction(action);

        action = new KAction(m_actionCollection);
        action->setSeparator(true);
        menu->addAction(action);

        action = new KAction(i18n("Reload Frame"), this);
        m_actionCollection->addAction(QL1S("reloadframe"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotReloadFrame()));
        menu->addAction(action);

        action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
        m_actionCollection->addAction(QL1S("printFrame"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(print()));
        menu->addAction(action);

        action = new KAction(i18n("Save &Frame As..."), this);
        m_actionCollection->addAction(QL1S("saveFrame"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotSaveFrame()));
        menu->addAction(action);

        action = new KAction(i18n("View Frame Source"), this);
        m_actionCollection->addAction(QL1S("viewFrameSource"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotViewFrameSource()));
        menu->addAction(action);
///TODO Slot not implemented yet
//         action = new KAction(i18n("View Frame Information"), this);
//         m_actionCollection->addAction(QL1S("viewFrameInfo"), action);
//         connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotViewPageInfo()));

        action = new KAction(m_actionCollection);
        action->setSeparator(true);
        menu->addAction(action);

        if (WebKitSettings::self()->isAdFilterEnabled()) {
            action = new KAction(i18n("Block IFrame..."), this);
            m_actionCollection->addAction(QL1S("blockiframe"), action);
            connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotBlockIFrame()));
            menu->addAction(action);
        }

        partActions.append(menu);
    }

    const bool showDocSourceAction = (!m_result.linkUrl().isValid() &&
                                      !m_result.imageUrl().isValid() &&
                                      !m_result.isContentSelected());
    const bool showInspectorAction = settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled);

    if (showDocSourceAction || showInspectorAction) {
        KAction *separatorAction = new KAction(m_actionCollection);
        separatorAction->setSeparator(true);
        partActions.append(separatorAction);
    }

    if (showDocSourceAction)
        partActions.append(m_part.data()->actionCollection()->action("viewDocumentSource"));

    if (showInspectorAction) {
        if (!m_webInspector) {
            m_webInspector = new QWebInspector;
            m_webInspector->setPage(page());
            connect(page(), SIGNAL(destroyed()), m_webInspector, SLOT(deleteLater()));
        }
        partActions.append(pageAction(QWebPage::InspectElement));
    } else {
        if (m_webInspector) {
            delete m_webInspector;
            m_webInspector = 0;
        }
    }

    partGroupMap.insert("partactions", partActions);
}

void WebView::selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& selectGroupMap)
{
    QList<QAction*> selectActions;

    KAction* copyAction = m_actionCollection->addAction(KStandardAction::Copy, "copy",  m_part.data()->browserExtension(), SLOT(copy()));
    copyAction->setText(i18n("&Copy Text"));
    copyAction->setEnabled(m_part.data()->browserExtension()->isActionEnabled("copy"));
    selectActions.append(copyAction);

    addSearchActions(selectActions, this);

    KUriFilterData data (selectedText().simplified().left(256));
    data.setCheckForExecutables(false);
    if (KUriFilter::self()->filterUri(data, QStringList() << "kshorturifilter" << "fixhosturifilter") &&
        data.uri().isValid() && data.uriType() == KUriFilterData::NetProtocol) {
        KAction *action = new KAction(KIcon("window-new"), i18nc("open selected url", "Open '%1'",
                                            KStringHandler::rsqueeze(data.uri().url(), 18)), this);
        m_actionCollection->addAction(QL1S("openSelection"), action);
        action->setData(QUrl(data.uri()));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(slotOpenSelection()));
        selectActions.append(action);
    }

    selectGroupMap.insert("editactions", selectActions);
}

void WebView::linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& linkGroupMap)
{
    Q_ASSERT(!m_result.linkUrl().isEmpty());

    const KUrl url(m_result.linkUrl());

    QList<QAction*> linkActions;

    KAction* action;

    if (m_result.isContentSelected()) {
        action = m_actionCollection->addAction(KStandardAction::Copy, "copy",  m_part.data()->browserExtension(), SLOT(copy()));
        action->setText(i18n("&Copy Text"));
        action->setEnabled(m_part.data()->browserExtension()->isActionEnabled("copy"));
        linkActions.append(action);
    }

    if (url.protocol() == "mailto") {
        action = new KAction(i18n("&Copy Email Address"), this);
        m_actionCollection->addAction(QL1S("copylinklocation"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotCopyLinkURL()));
        linkActions.append(action);
    } else {
        action = new KAction(i18n("&Copy Link URL"), this);
        m_actionCollection->addAction(QL1S("copylinkurl"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotCopyLinkURL()));
        linkActions.append(action);

        action = new KAction(i18n("&Save Link As..."), this);
        m_actionCollection->addAction(QL1S("savelinkas"), action);
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(slotSaveLinkAs()));
        linkActions.append(action);
    }
    linkGroupMap.insert("linkactions", linkActions);
}

void WebView::multimediaActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& mmGroupMap)
{
    QList<QAction*> multimediaActions;

    QWebElement element (m_result.element());
    const bool isPaused = element.evaluateJavaScript(QL1S("this.paused")).toBool();
    const bool isMuted = element.evaluateJavaScript(QL1S("this.muted")).toBool();
    const bool isLoopOn = element.evaluateJavaScript(QL1S("this.loop")).toBool();
    const bool areControlsOn = element.evaluateJavaScript(QL1S("this.controls")).toBool();
    const bool isVideoElement = (element.tagName().compare(QL1S("video"), Qt::CaseInsensitive) == 0);
    const bool isAudioElement = (element.tagName().compare(QL1S("audio"), Qt::CaseInsensitive) == 0);

    KAction* action = new KAction((isPaused ? i18n("&Play") : i18n("&Pause")), this);
    m_actionCollection->addAction(QL1S("playmultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part.data()->browserExtension(), SLOT(slotPlayMedia()));
    multimediaActions.append(action);

    action = new KAction((isMuted ? i18n("Un&mute") : i18n("&Mute")), this);
    m_actionCollection->addAction(QL1S("mutemultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part.data()->browserExtension(), SLOT(slotMuteMedia()));
    multimediaActions.append(action);

    action = new KAction(i18n("&Loop"), this);
    action->setCheckable(true);
    action->setChecked(isLoopOn);
    m_actionCollection->addAction(QL1S("loopmultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part.data()->browserExtension(), SLOT(slotLoopMedia()));
    multimediaActions.append(action);

    action = new KAction(i18n("Show &Controls"), this);
    action->setCheckable(true);
    action->setChecked(areControlsOn);
    m_actionCollection->addAction(QL1S("showmultimediacontrols"), action);
    connect(action, SIGNAL(triggered()), m_part.data()->browserExtension(), SLOT(slotShowMediaControls()));
    multimediaActions.append(action);

    action = new KAction(m_actionCollection);
    action->setSeparator(true);
    multimediaActions.append(action);

    QString saveMediaText, copyMediaText;
    if (isVideoElement) {
        saveMediaText = i18n("Sa&ve Video As...");
        copyMediaText = i18n("C&opy Video URL");
    } else if (isAudioElement) {
        saveMediaText = i18n("Sa&ve Audio As...");
        copyMediaText = i18n("C&opy Audio URL");
    } else {
        saveMediaText = i18n("Sa&ve Media As...");
        copyMediaText = i18n("C&opy Media URL");
    }

    action = new KAction(saveMediaText, this);
    m_actionCollection->addAction(QL1S("savemultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part.data()->browserExtension(), SLOT(slotSaveMedia()));
    multimediaActions.append(action);

    action = new KAction(copyMediaText, this);
    m_actionCollection->addAction(QL1S("copymultimediaurl"), action);
    connect(action, SIGNAL(triggered()), m_part.data()->browserExtension(), SLOT(slotCopyMedia()));
    multimediaActions.append(action);

    mmGroupMap.insert("partactions", multimediaActions);
}

void WebView::slotOpenSelection()
{
    QAction *action = qobject_cast<KAction*>(sender());
    if (action) {
        KParts::BrowserArguments browserArgs;
        browserArgs.frameName = "_blank";
        emit m_part.data()->browserExtension()->openUrlRequest(action->data().toUrl(), KParts::OpenUrlArguments(), browserArgs);
    }
}

void WebView::addSearchActions(QList<QAction*>& selectActions, QWebView* view)
{
   // search text
    const QString selectedText = view->selectedText().simplified();
    if (selectedText.isEmpty())
        return;

    KUriFilterData data;
    data.setData(selectedText);
    data.setAlternateDefaultSearchProvider(ALTERNATE_DEFAULT_PROVIDER);
    data.setAlternateSearchProviders(ALTERNATE_SEARCH_PROVIDERS);

    if (KUriFilter::self()->filterSearchUri(data, KUriFilter::NormalTextFilter)) {
        const QString squeezedText = KStringHandler::rsqueeze(selectedText, 20);
        KAction *action = new KAction(KIcon(data.iconName()),
                                      i18nc("Search \"search provider\" for \"text\"", "Search %1 for '%2'",
                                            data.searchProvider(), squeezedText), view);
        action->setData(QUrl(data.uri()));
        connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(searchProvider()));
        m_actionCollection->addAction(QL1S("defaultSearchProvider"), action);
        selectActions.append(action);


        const QStringList preferredSearchProviders = data.preferredSearchProviders();
        if (!preferredSearchProviders.isEmpty()) {
            KActionMenu* providerList = new KActionMenu(i18nc("Search for \"text\" with",
                                                              "Search for '%1' with", squeezedText), view);
            Q_FOREACH(const QString &searchProvider, preferredSearchProviders) {
                if (searchProvider == data.searchProvider())
                    continue;

                KAction *action = new KAction(KIcon(data.iconNameForPreferredSearchProvider(searchProvider)), searchProvider, view);
                action->setData(data.queryForPreferredSearchProvider(searchProvider));
                m_actionCollection->addAction(searchProvider, action);
                connect(action, SIGNAL(triggered(bool)), m_part.data()->browserExtension(), SLOT(searchProvider()));

                providerList->addAction(action);
            }
            m_actionCollection->addAction(QL1S("searchProviderList"), providerList);
            selectActions.append(providerList);
        }
    }
}
