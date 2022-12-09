#include "tukiwidget.h"
#include "ui_tukiwidget.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "debugtiedotdlg.h"

TukiWidget::TukiWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TukiWidget)
{
    ui->setupUi(this);

    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &TukiWidget::kirjauduttu);

    connect( ui->ohjeNappi, &QPushButton::clicked, this, &TukiWidget::ohjeet);
    connect( ui->tukiNappi, &QPushButton::clicked, this, &TukiWidget::tuki);
    connect( ui->palauteNappi, &QPushButton::clicked, this, &TukiWidget::palaute);
    connect( ui->debugNappi, &QPushButton::clicked, this, &TukiWidget::virheenjaljitys);

    if( kp()->pilvi()->kayttaja()) {
        kirjauduttu( kp()->pilvi()->kayttaja() );
    }
}

TukiWidget::~TukiWidget()
{
    delete ui;
}

void TukiWidget::kirjauduttu(PilviKayttaja kayttaja)
{
    bool tilaaja =
            kayttaja.planId() > 0 ||
            kayttaja.trialPeriod().isValid() ||
            kayttaja.moodi() == PilviKayttaja::PRO;

    ui->tukiNappi->setEnabled( tilaaja );
    ui->debugNappi->setEnabled( kayttaja );

    ui->tunnusGroup->setVisible( kayttaja );
    ui->idLabel->setText( QString("%1").arg(kayttaja.id()));
    ui->nameLabel->setText(QString("%1\n%2").arg(kayttaja.nimi(), kayttaja.email()));
}

void TukiWidget::ohjeet()
{
    kp()->ohje();
}

void TukiWidget::tuki()
{
    const QString url = kp()->pilvi()->service("support");
    kp()->avaaUrl( url );
}

void TukiWidget::palaute()
{
    const QString url = kp()->pilvi()->service("feedback");
    kp()->avaaUrl( url.isEmpty() ? "https://kitsas.fi/palaute" : url );
}

void TukiWidget::virheenjaljitys()
{
    DebugTiedotDlg dlg(this);
    dlg.exec();
}
