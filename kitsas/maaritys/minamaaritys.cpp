#include "minamaaritys.h"

#include "ui_minamaaritys.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikayttaja.h"

#include "aloitussivu/salasananvaihto.h"
#include "tilaus/tilauswizard.h"
#include "tilaus/laskutustietodialog.h"
#include "aloitussivu/kaksivaihedialog.h"

#include <QMessageBox>

MinaMaaritys::MinaMaaritys() :
    ui{ new Ui::MinaMaaritys }
{
    ui->setupUi(this);
    connect( ui->salasanaNappi, &QPushButton::clicked, this, &MinaMaaritys::vaihdaSalasana);
    connect( ui->normaaliRadio, &QRadioButton::toggled, this, &MinaMaaritys::paivitaMoodi);
    connect( ui->toffeeRadio, &QRadioButton::toggled, this, &MinaMaaritys::paivitaMoodi);
    connect( ui->tilausButton, &QPushButton::clicked, this, &MinaMaaritys::tilausNappi);
    connect( ui->laskutusButton, &QPushButton::clicked, this, &MinaMaaritys::laskutusTiedot);

    connectMuutokset();
}

MinaMaaritys::~MinaMaaritys()
{
    delete ui;
}

bool MinaMaaritys::nollaa()
{
    PilviKayttaja kayttaja = kp()->pilvi()->kayttaja();

    hae();

    if( kayttaja.planId()) {
        QString info = kayttaja.planName();
        if( kayttaja.cloudCount()) info.append("\n" + tr("Käytössä %1 kirjanpitoa").arg(kayttaja.cloudCount()));
        if( kayttaja.capacity()) info.append("\n" + tr("Tilaukseen sisältyy %1, jonka jälkeen lisäkirjanpidot maksavat %2 /kk")
                                                  .arg(kayttaja.capacity() == 1 ? tr("yksi kirjanpito") : tr("%1 kirjanpitoa").arg(kayttaja.capacity()), kayttaja.extraMonthly().display(true))
                                             );
        ui->tilausInfo->setText( info );
        ui->tilausButton->setText( tr("Tilaukseni"));
    } else if( kayttaja.trialPeriod().isValid()) {
        ui->tilausInfo->setText(tr("Kokeilujakso %1 saakka").arg(kayttaja.trialPeriod().toString("dd.MM.yyyy")));
    } else {
        ui->tilausInfo->setText( tr("Ei voimassa olevaa tilausta") );
    }

    return true;
}

bool MinaMaaritys::onkoMuokattu()
{
    return
            ui->nimiEdit->text() != minaMap_.value("name").toString() ||
            ui->emailEdit->text() != minaMap_.value("email").toString() ||
            ui->phoneEdit->text() != minaMap_.value("phone").toString() ||
            ui->kayta2fa->isChecked() != minaMap_.value("use2fa").toBool() ||
            ui->toffeeRadio->isChecked() != minaMap_.value("pro").toBool() ||
            paivalista() != minaMap_.value("emaildays").toList();
}

bool MinaMaaritys::tallenna()
{
    QVariantMap tallennus;

    if( ui->nimiEdit->text() != minaMap_.value("name").toString())
        tallennus.insert("name", ui->nimiEdit->text());
    if( ui->emailEdit->text() != minaMap_.value("email").toString())
        tallennus.insert("email", ui->emailEdit->text());
    if( ui->phoneEdit->text() != minaMap_.value("phone").toString())
        tallennus.insert("phone", ui->phoneEdit->text());
    if(ui->kayta2fa->isChecked() != minaMap_.value("use2fa").toBool())
        tallennus.insert("twofa", ui->kayta2fa->isChecked());
    if( ui->toffeeRadio->isChecked() != minaMap_.value("pro").toBool())
        tallennus.insert("pro", ui->toffeeRadio->isChecked());
    if( paivalista() != minaMap_.value("emaildays").toList())
        tallennus.insert("emaildays", paivalista());



    KpKysely* kysymys = kp()->pilvi()->loginKysely("/me", KpKysely::PATCH);
    connect( kysymys, &KpKysely::vastaus, this, &MinaMaaritys::tallennettu);
    kysymys->kysy(tallennus);

    return false;
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

void MinaMaaritys::hae()
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely("/me");
    connect( kysymys, &KpKysely::vastaus, this, &MinaMaaritys::lueVastaus);
    kysymys->kysy();
}

void MinaMaaritys::lueVastaus(QVariant *data)
{
    minaMap_ = data->toMap();

    ui->nimiEdit->setText( minaMap_.value("name").toString() );
    ui->emailEdit->setText( minaMap_.value("email").toString() );
    ui->phoneEdit->setText( minaMap_.value("phone").toString());

    bool proMode = minaMap_.value("pro").toBool();
    ui->normaaliRadio->setChecked( !proMode );
    ui->toffeeRadio->setChecked( proMode );

    // Näkymä piiloon jos esim. asiakastyyppinen tunnari
    bool naytaNakyma = !minaMap_.contains("mode") ||
                        minaMap_.value("mode") == "NORMAL" ||
                        minaMap_.value("mode") == "PRO";
    ui->nakymaGroup->setVisible(naytaNakyma);

    ui->kayta2fa->setChecked( minaMap_.value("use2fa").toBool() );

    QVariantList viikonpaivat = minaMap_.value("emaildays").toList();
    ui->ilmoSu->setChecked( viikonpaivat.contains(0) );
    ui->ilmoMa->setChecked( viikonpaivat.contains(1) );
    ui->ilmoTi->setChecked( viikonpaivat.contains(2) );
    ui->ilmoKe->setChecked( viikonpaivat.contains(3) );
    ui->ilmoTo->setChecked( viikonpaivat.contains(4) );
    ui->ilmoPe->setChecked( viikonpaivat.contains(5) );
    ui->ilmoLa->setChecked( viikonpaivat.contains(6) );

    paivitaMoodi();
    tarkastaMuokkaus();
}

void MinaMaaritys::tallennettu(QVariant *data)
{
    QVariantMap map = data->toMap();

    if( map.contains("url2fa")) {
        kp()->avaaUrl(map.value("url2fa").toUrl());
        QMessageBox::information(this, tr("Kaksivaiheinen tunnistautuminen"),
                                 tr("Ota kaksivaiheinen tunnistautuminen käyttöön selaimeesi avautuvan QR-koodin avulla."));
    }

    if( map.contains("keyid")) {
        keyid_ = map.value("keyid").toString();
    }

    if( map.value("email").toString() == "ERROR" ) {
        QMessageBox::critical(this, tr("Sähköpostiosoitteen vaihtaminen"),
                              tr("Sähköpostiosoitteen vaihtaminen epäonnistui.") + "\n" +
                              tr("Tällä sähköpostiosoitteella on jo mahdollisesti Kitsaan käyttäjätunnus."));
        keyid_.clear();
    }

    if( map.contains("keyid") || map.value("email").toString() == "FAIL") {
        KaksivaiheDialog dlg(this);
        QString koodi = dlg.askEmailCode(ui->emailEdit->text());
        if( !koodi.isEmpty() ) {
            QVariantMap kmap;
            kmap.insert("keyid", keyid_);
            kmap.insert("code", koodi);
            KpKysely* kysely = kp()->pilvi()->loginKysely("/me", KpKysely::PATCH);
            connect( kysely, &KpKysely::vastaus, this, &MinaMaaritys::tallennettu);
            kysely->kysy(kmap);
        } else {
            keyid_.clear();
        }
    } else {
        nollaa();
    }
}

QVariantList MinaMaaritys::paivalista() const
{
    QVariantList lista;
    if( ui->ilmoSu->isChecked()) lista << 0;
    if( ui->ilmoMa->isChecked()) lista << 1;
    if( ui->ilmoTi->isChecked()) lista << 2;
    if( ui->ilmoKe->isChecked()) lista << 3;
    if( ui->ilmoTo->isChecked()) lista << 4;
    if( ui->ilmoPe->isChecked()) lista << 5;
    if( ui->ilmoLa->isChecked()) lista << 6;
    return lista;
}
