#include "minamaaritys.h"

#include "ui_minamaaritys.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikayttaja.h"

#include "aloitussivu/salasananvaihto.h"
#include "tilaus/tilauswizard.h"
#include "tilaus/laskutustietodialog.h"

MinaMaaritys::MinaMaaritys() :
    ui{ new Ui::MinaMaaritys }
{
    ui->setupUi(this);
    connect( ui->salasanaNappi, &QPushButton::clicked, this, &MinaMaaritys::vaihdaSalasana);
    connect( ui->normaaliRadio, &QRadioButton::toggled, this, &MinaMaaritys::paivitaMoodi);
    connect( ui->toffeeRadio, &QRadioButton::toggled, this, &MinaMaaritys::paivitaMoodi);
    connect( ui->tilausButton, &QPushButton::clicked, this, &MinaMaaritys::tilausNappi);
    connect( ui->laskutusButton, &QPushButton::clicked, this, &MinaMaaritys::laskutusTiedot);
}

MinaMaaritys::~MinaMaaritys()
{
    delete ui;
}

bool MinaMaaritys::nollaa()
{
    PilviKayttaja kayttaja = kp()->pilvi()->kayttaja();

    ui->nimiEdit->setText( kayttaja.nimi() );
    ui->emailEdit->setText( kayttaja.email() );
    ui->phoneEdit->setText( kayttaja.phone());

    ui->normaaliRadio->setChecked( kayttaja.moodi() == PilviKayttaja::NORMAALI );
    ui->toffeeRadio->setChecked( kayttaja.moodi() == PilviKayttaja::TOFFEE );

    ui->kayta2fa->setChecked( kayttaja.with2FA() );

    if( kayttaja.planId()) {
        ui->tilausInfo->setText( kayttaja.planName() );
        ui->tilausButton->setText( tr("Tilaukseni"));
    } else if( kayttaja.trialPeriod().isValid()) {
        ui->tilausInfo->setText(tr("Kokeilujakso %1 saakka").arg(kayttaja.trialPeriod().toString("dd.MM.yyyy")));
    } else {
        ui->tilausInfo->setText( tr("Ei voimassa olevaa tilausta") );
    }

    paivitaMoodi();
    return true;
}

void MinaMaaritys::paivitaMoodi()
{
    ui->tilausGroup->setVisible( ui->normaaliRadio->isChecked() );
}

void MinaMaaritys::vaihdaSalasana()
{
    Salasananvaihto dlg(this);
    dlg.exec();
}

void MinaMaaritys::tilausNappi()
{
    TilausWizard *tilaus = new TilausWizard;
    tilaus->nayta();
}

void MinaMaaritys::laskutusTiedot()
{
    LaskutustietoDialog dlg(this);
    dlg.exec();
}
