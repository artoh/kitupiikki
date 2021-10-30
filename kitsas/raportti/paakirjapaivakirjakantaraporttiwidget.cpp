#include "paakirjapaivakirjakantaraporttiwidget.h"

#include "ui_paivakirja.h"
#include "naytin/naytinikkuna.h"

#include "db/kirjanpito.h"

PaakirjaPaivakirjaKantaRaporttiWidget::PaakirjaPaivakirjaKantaRaporttiWidget() :
    RaporttiWidget(nullptr)
{
    ui = new Ui::Paivakirja;
    ui->setupUi(raporttiWidget);

    ui->alkupvm->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::AlkuPvm).toDate() );
    ui->loppupvm->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::LoppuPvm).toDate() );

    ui->ryhmittelelajeittainCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::RyhmitteleTositelajit) );
    ui->tulostakohdennuksetCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaKohdennus) );
    ui->tulostasummat->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaSummarivit));
    ui->kumppaniCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaKumppani));
    ui->eriPaivatCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::ErittelePaivat));

    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KAIKKI);

    if( !kp()->kohdennukset()->kohdennuksia()) {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    } else {
        paivitaKohdennukset();
        int kohdennus = kp()->raporttiValinnat()->arvo(RaporttiValinnat::Kohdennuksella).toInt();
        ui->kohdennusCheck->setChecked(kohdennus > -1);
        if( kohdennus > -1)
            ui->kohdennusCombo->valitseKohdennus(kohdennus);
    }

    ui->kieliCombo->valitse( kp()->raporttiValinnat()->arvo(RaporttiValinnat::Kieli).toString() );

    connect( ui->alkupvm, &QDateEdit::dateChanged, this, &PaakirjaPaivakirjaKantaRaporttiWidget::paivitaKohdennukset);
    connect( ui->loppupvm, &QDateEdit::dateChanged, this, &PaakirjaPaivakirjaKantaRaporttiWidget::paivitaKohdennukset);
}

PaakirjaPaivakirjaKantaRaporttiWidget::~PaakirjaPaivakirjaKantaRaporttiWidget()
{
    delete ui;
}

void PaakirjaPaivakirjaKantaRaporttiWidget::esikatsele()
{
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::AlkuPvm, ui->alkupvm->date());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::LoppuPvm, ui->loppupvm->date());

    if( ui->kohdennusCheck->isVisible() && ui->kohdennusCheck->isChecked()) {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Kohdennuksella, ui->kohdennusCombo->kohdennus());
    } else {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Kohdennuksella, -1);
    }

    if( ui->tositejarjestysRadio->isChecked() )
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::VientiJarjestys, "tosite");
    else
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::VientiJarjestys, "pvm");

    kp()->raporttiValinnat()->aseta(RaporttiValinnat::RyhmitteleTositelajit, ui->ryhmittelelajeittainCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::TulostaKohdennus, ui->tulostakohdennuksetCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::TulostaSummarivit, ui->tulostasummat->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::TulostaKumppani, ui->kumppaniCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::ErittelePaivat, ui->eriPaivatCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::Kieli, ui->kieliCombo->currentData().toString());

    if( ui->tiliBox->isChecked()) {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Tililta, ui->tiliCombo->currentData());
    } else {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Tililta, QVariant());
    }

    tallennaValinnat();
    NaytinIkkuna::naytaRaportti( *kp()->raporttiValinnat());
}

void PaakirjaPaivakirjaKantaRaporttiWidget::paivitaKohdennukset()
{
    ui->kohdennusCombo->suodataValilla( ui->alkupvm->date(), ui->loppupvm->date() );
}

