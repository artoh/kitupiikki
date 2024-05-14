#include "hubtoimistosivu.h"

#include <QWebEngineView>
#include <QHBoxLayout>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "hubwebpage.h"
#include <QtGlobal>
#include <QApplication>
#include <QDesktopServices>

HubToimistoSivu::HubToimistoSivu(QWidget* parent, Jarjestelma jarjestelma)
    :  KitupiikkiSivu(parent),    
    jarjestelma_(jarjestelma)
{

}



void HubToimistoSivu::siirrySivulle()
{    
    if( qApp->property("noweb").toBool()) {
        QString url = kp()->pilvi()->service(jarjestelma_ == MAJAVA ? "majava" : "admin");
        url.replace("client=desktop","client=web");
        QDesktopServices::openUrl(url);
    } else if( !view_ ) {
        alusta();
        naytaSivu();
    } else if( jarjestelma_ == ADMIN || view_->url().isEmpty()) {
        naytaSivu();
    }
}

void HubToimistoSivu::alustaSivu()
{
    if( view_) {
        view_->load(QUrl("qrc:/loading.html"));
        if( jarjestelma_ == MAJAVA ) {
            const QString url = kp()->pilvi()->service("majava");
            if(!url.isEmpty()) view_->load(url);
        }
    }
}

void HubToimistoSivu::naytaSivu()
{
    view_->load(QUrl("qrc:/loading.html"));
    const QString url = kp()->pilvi()->service(jarjestelma_ == MAJAVA ? "majava" : "admin");
    if( !url.isEmpty()) view_->load(url);
}

void HubToimistoSivu::alusta()
{
    HubWebPage* webPage = new HubWebPage(this);
    view_ = new QWebEngineView(webPage, this);

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(view_);
    setLayout(layout);

    view_->load(QUrl("qrc:/loading.html"));
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &HubToimistoSivu::alustaSivu);

    if( jarjestelma_ == MAJAVA ) {
        connect( webPage, &HubWebPage::toimistoLinkki, this, &HubToimistoSivu::toimistoLinkki);
    }
}

void HubToimistoSivu::naytaToimisto(const QString &id)
{    
    view_->load(QUrl("qrc:/loading.html"));
    const QString url = kp()->pilvi()->service("admin");
    if( !url.isEmpty()) view_->load(url + "&office=" + id);
}


