#include "hubtoimistosivu.h"

#include <QWebEngineView>
#include <QHBoxLayout>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "hubwebpage.h"
#include <QtGlobal>

HubToimistoSivu::HubToimistoSivu(QWidget* parent, Jarjestelma jarjestelma)
    :  KitupiikkiSivu(parent),    
    jarjestelma_(jarjestelma)
{
#ifdef Q_OS_WIN
    alusta();
#endif
}



void HubToimistoSivu::siirrySivulle()
{
    if( !view_ ) {
        alusta();
        alustaSivu();
    }
}

void HubToimistoSivu::alustaSivu()
{
    if( view_) {
        view_->load(QUrl("qrc:/loading.html"));
        const QString url = kp()->pilvi()->service(jarjestelma_ == MAJAVA ? "majava" : "admin");
        view_->load(url);
    }
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
    view_->load(url + "&office=" + id);
}


