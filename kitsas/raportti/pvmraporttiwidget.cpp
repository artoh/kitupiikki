#include "pvmraporttiwidget.h"

#include "ui_paivakirja.h"
#include "db/kirjanpito.h"
#include "eranselvittaja/eranselvitys.h"
#include "pilvi/pilvimodel.h"

PvmRaporttiWidget::PvmRaporttiWidget(const QString &tyyppi) :
    RaporttiWidget(nullptr), tyyppi_(tyyppi)
{
    ui = new Ui::Paivakirja;
    ui->setupUi(raporttiWidget);

    lataa();
    piilotaTarpeettomat();
}

PvmRaporttiWidget::~PvmRaporttiWidget()
{
    delete ui;
}

void PvmRaporttiWidget::lataa()
{
    ui->alkupvm->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::AlkuPvm).toDate() );
    ui->loppupvm->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::LoppuPvm).toDate() );

    ui->ryhmittelelajeittainCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::RyhmitteleTositelajit) );
    ui->tulostakohdennuksetCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaKohdennus) );
    ui->tulostasummat->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaSummarivit));
    ui->kumppaniCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaKumppani));
    ui->eriPaivatCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::ErittelePaivat));
    ui->piilotaNollaCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::PiilotaNollasaldot) );
    ui->erittelemattomatCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::ErittelemattomienViennit) );
    ui->alvCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::NaytaAlvProsentti) );

    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KAIKKI);
    paivita();

    if( !kp()->kohdennukset()->kohdennuksia()) {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    } else {        
        int kohdennus = kp()->raporttiValinnat()->arvo(RaporttiValinnat::Kohdennuksella).toInt();
        ui->kohdennusCheck->setChecked(kohdennus > -1);
        if( kohdennus > -1)
            ui->kohdennusCombo->valitseKohdennus(kohdennus);
    }

    ui->kieliCombo->valitse( kp()->raporttiValinnat()->arvo(RaporttiValinnat::Kieli).toString() );

    connect( ui->alkupvm, &KpDateEdit::dateChanged, this, &PvmRaporttiWidget::paivita);
    connect( ui->loppupvm, &KpDateEdit::dateChanged, this, &PvmRaporttiWidget::paivita);

    connect( ui->selvittelyNappi, &QPushButton::clicked, this, &PvmRaporttiWidget::taseSelvittely);

}

void PvmRaporttiWidget::paivita()
{
    ui->kohdennusCombo->suodataValilla( ui->alkupvm->date(), ui->loppupvm->date() );

    KpKysely *kysely = kpk("/saldot");
    kysely->lisaaAttribuutti("alkupvm", ui->alkupvm->date());
    kysely->lisaaAttribuutti("pvm", ui->loppupvm->date());
    connect(kysely, &KpKysely::vastaus, this, &PvmRaporttiWidget::tiliListaSaapuu);
    kysely->kysy();
}

void PvmRaporttiWidget::piilotaTarpeettomat()
{
    ui->laatuLabel->hide();
    ui->laatuSlider->hide();
    ui->poimiGroup->hide();

    if( tyyppi() != "paivakirja" && tyyppi() != "tositeluettelo") {
        ui->jarjestysRyhma->hide();
        ui->ryhmittelelajeittainCheck->hide();
        ui->eriPaivatCheck->hide();
    }

    if( tyyppi() != "paakirja") {
        ui->tiliBox->hide();
        ui->tiliCombo->hide();
    }

    if( tyyppi() != "paivakirja" && tyyppi() != "paakirja") {
        ui->kohdennusCheck->hide();
        ui->kohdennusCombo->hide();
        ui->tulostakohdennuksetCheck->hide();
        ui->alvCheck->hide();
    }    

    if( tyyppi() != "paivakirja" && tyyppi() != "paakirja" && tyyppi() != "tositeluettelo") {
        ui->kumppaniCheck->hide();
        ui->tulostasummat->hide();        
    }

    if( tyyppi() != "paivakirja" && tyyppi() != "tositeluettelo") {
        ui->eriPaivatCheck->hide();        
    }

    const bool pilvessa = qobject_cast<PilviModel*>(kp()->yhteysModel()) != nullptr;
    if( tyyppi() != "taseerittely" || !pilvessa) {
        ui->selvittelyNappi->hide();
        ui->piilotaNollaCheck->hide();
        ui->erittelemattomatCheck->hide();
    }

    if( !kp()->asetukset()->onko(AsetusModel::AlvVelvollinen))
        ui->alvCheck->hide();   // Alv-ruksi ei käytössä jos ei alv-velvollinen
}

void PvmRaporttiWidget::taseSelvittely()
{
    EranSelvitys* selvitys = new EranSelvitys( ui->alkupvm->date(), ui->loppupvm->date() );
    selvitys->show();
}

void PvmRaporttiWidget::tallenna()
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
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::PiilotaNollasaldot, ui->piilotaNollaCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::ErittelemattomienViennit, ui->erittelemattomatCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::NaytaAlvProsentti, ui->alvCheck->isChecked());

    if( ui->tiliBox->isChecked()) {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Tililta, ui->tiliCombo->currentData());
    } else {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Tililta, QVariant());
    }

    kp()->raporttiValinnat()->aseta(RaporttiValinnat::Tyyppi, tyyppi());
}

void PvmRaporttiWidget::tiliListaSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    const int valittuTili = ui->tiliCombo->currentData().toInt();

    ui->tiliCombo->clear();
    for( const QString& tiliStr : map.keys()) {
        const Tili& tili = kp()->tilit()->tiliNumerolla( tiliStr.toInt() );
        ui->tiliCombo->addItem( tili.nimiNumero(), tili.numero() );
    }
    const int valitunIndeksi = ui->tiliCombo->findData(valittuTili);
    if( valitunIndeksi)
        ui->tiliCombo->setCurrentIndex(valitunIndeksi);

    ui->tiliCombo->model()->sort(0);
}
