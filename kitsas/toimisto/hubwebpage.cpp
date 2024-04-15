
#include "hubwebpage.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QDesktopServices>
#include <QWebEngineNewWindowRequest>

HubWebPage::HubWebPage(QObject *parent)
    : QWebEnginePage{parent}
{
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

