#include "lisapalvelutmaaritys.h"

#include <QBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/avattupilvi.h"
#include "pilvi/pilviextra.h"


LisaPalvelutMaaritys::LisaPalvelutMaaritys()
{
    setupUi();
}

void LisaPalvelutMaaritys::setupUi()
{
    QLabel* otsikko = new QLabel("Lisäpalvelut");
    otsikko->setStyleSheet("font-weight: bold");
    QScrollArea* skrolli = new QScrollArea;
    QVBoxLayout* skrolliLeiska = new QVBoxLayout;
    QWidget* sisalto = new QWidget();
    sisalto->setLayout(skrolliLeiska);
    skrolli->setWidget(sisalto);
    skrolli->setWidgetResizable(true);

    const AvattuPilvi pilvi = kp()->pilvi()->pilvi();



    QList<int> palvelut = pilvi.extrat();
    for(const int palvelu : palvelut) {
        PilviExtra ekstra = pilvi.extra(palvelu);

        QGroupBox* box = new QGroupBox();
        QHBoxLayout* eleiska = new QHBoxLayout();
        QVBoxLayout* etleiska = new QVBoxLayout();

        QLabel* nlabel = new QLabel( ekstra.title() );
        nlabel->setStyleSheet("font-weight: bold");
        etleiska->addWidget(nlabel);
        QLabel* xlabel = new QLabel( ekstra.active() ? ekstra.statusinfo() : ekstra.description() );
        etleiska->addWidget( xlabel );

        eleiska->addLayout(etleiska);
        eleiska->addStretch();

        QPushButton* button = ekstra.active() ?
                    new QPushButton(QIcon(":/pic/poista.png"), tr("Poista käytöstä")) :
                    new QPushButton(QIcon(":/pic/lisaa.png"), tr("Ota käyttöön")) ;


        eleiska->addWidget(button);

        box->setLayout(eleiska);
        skrolliLeiska->setSizeConstraint(QLayout::SetMinimumSize);
        skrolliLeiska->addWidget(box);
    }

    skrolliLeiska->addStretch();

    QVBoxLayout* leiska = new QVBoxLayout();
    leiska->addWidget(otsikko);
    leiska->addWidget(skrolli);
    setLayout(leiska);
}
