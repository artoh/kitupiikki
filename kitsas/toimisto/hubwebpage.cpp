
#include "hubwebpage.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "qaction.h"

#include <QDesktopServices>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineProfile>

HubWebPage::HubWebPage(QObject *parent)
    : QWebEnginePage{parent}
{
    action(QWebEnginePage::SavePage)->setVisible(false);
    action(QWebEnginePage::Reload)->setVisible(false);
    action(QWebEnginePage::ViewSource)->setVisible(false);
    action(QWebEnginePage::Forward)->setText(tr("Seuraava sivu"));
    action(QWebEnginePage::Back)->setText(tr("Edellinen sivu"));
    action(QWebEnginePage::OpenLinkInNewTab)->setVisible(false);
    action(QWebEnginePage::OpenLinkInNewWindow)->setVisible(false);
    action(QWebEnginePage::CopyLinkToClipboard)->setVisible(false);
    action(QWebEnginePage::CopyImageUrlToClipboard)->setVisible(false);
    action(QWebEnginePage::CopyImageToClipboard)->setVisible(false);
    action(QWebEnginePage::Paste)->setText(tr("Liitä"));
    action(QWebEnginePage::PasteAndMatchStyle)->setVisible(false);
    action(QWebEnginePage::Copy)->setText(tr("Kopioi"));
    action(QWebEnginePage::Cut)->setText(tr("Leikkaa"));
    action(QWebEnginePage::SelectAll)->setVisible(false);
    action(QWebEnginePage::DownloadImageToDisk)->setVisible(false);
    action(QWebEnginePage::DownloadLinkToDisk)->setVisible(false);
    action(QWebEnginePage::Undo)->setText(tr("kumoa"));
    action(QWebEnginePage::Redo)->setText(tr("Tee uudelleen"));

    connect( this, &HubWebPage::newWindowRequested, this, &HubWebPage::openLinkInNewWindow);

}

void HubWebPage::supportLogin(const QString &cloudId)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
        QString("/auth/support/%1").arg(cloudId));
    connect( kysymys, &KpKysely::vastaus, this, [] (QVariant* vastaus) { kp()->pilvi()->alustaPilvi(vastaus, false); });
    kysymys->kysy();
}

void HubWebPage::login(const QString &cloudId)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
        QString("/auth/%1").arg(cloudId));
    connect( kysymys, &KpKysely::vastaus, this, [] (QVariant* vastaus) { kp()->pilvi()->alustaPilvi(vastaus, false); });
    kysymys->kysy();
}

bool HubWebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool /*isMainFrame*/)
{
    if( url.scheme() == "kitsas") {
        const QString path = url.path();
        if(path.startsWith("SU-")) {
            supportLogin(path.mid(3));
        } else {
            login(path);
        }
        return false;
    } else if(url.scheme() == "office") {
        emit toimistoLinkki(url.path());
        return false;
    } else if( type == QWebEnginePage::NavigationTypeBackForward) {
        return url.scheme() == "http" || url.scheme() == "https";
    } else {
        return true;
    }
}

void HubWebPage::openLinkInNewWindow(QWebEngineNewWindowRequest &request)
{
    if( request.destination() == QWebEngineNewWindowRequest::InNewWindow || request.destination() == QWebEngineNewWindowRequest::InNewTab)
        QDesktopServices::openUrl(request.requestedUrl());
}



