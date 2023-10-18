#include "hubtoimistosivu.h"

#include <QWebEngineView>
#include <QHBoxLayout>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "hubwebpage.h"

HubToimistoSivu::HubToimistoSivu(QWidget* parent, Jarjestelma jarjestelma)
    :  KitupiikkiSivu(parent),    
    jarjestelma_(jarjestelma)
{
    HubWebPage* webPage = new HubWebPage(this);
    view_ = new QWebEngineView(webPage, this);

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(view_);
    setLayout(layout);

    view_->setHtml("<html><body></body></html>");
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &HubToimistoSivu::alustaSivu);

    if( jarjestelma == MAJAVA ) {
        connect( webPage, &HubWebPage::toimistoLinkki, this, &HubToimistoSivu::toimistoLinkki);
    }
}




void HubToimistoSivu::siirrySivulle()
{

}

void HubToimistoSivu::alustaSivu()
{
    const QString url = kp()->pilvi()->service(jarjestelma_ == MAJAVA ? "majava" : "admin");
    view_->load(url);
}

void HubToimistoSivu::naytaToimisto(const QString &id)
{
    const QString url = kp()->pilvi()->service("admin");
    view_->load(url + "&office=" + id);
}


