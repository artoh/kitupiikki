#include "hubtoimistosivu.h"

#include <QWebEngineView>
#include <QHBoxLayout>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

HubToimistoSivu::HubToimistoSivu(QWidget* parent, Jarjestelma jarjestelma)
    :  KitupiikkiSivu(parent),
    view_(new QWebEngineView(this)),
    jarjestelma_(jarjestelma)
{
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(view_);
    setLayout(layout);

    view_->setHtml("<html><body></body></html>");
}

void HubToimistoSivu::siirrySivulle()
{
    const QString url = kp()->pilvi()->service(jarjestelma_ == MAJAVA ? "majava" : "admin");
    view_->load(url);
}
