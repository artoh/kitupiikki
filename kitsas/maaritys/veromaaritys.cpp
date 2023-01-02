#include "veromaaritys.h"
#include "ui_veromaaritys.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/avattupilvi.h"

#include "alv/verovarmennetila.h"
#include "alv/varmennedialog.h"

#include <QTimer>
#include <QMessageBox>

#include <QRegularExpressionValidator>

#include "ui_maksuperusteinen.h"

VeroMaaritys::VeroMaaritys() :
    ui{new Ui::VeroMaaritys},
    tila(new VeroVarmenneTila(this))
{
    ui->setupUi(this);

    connect( ui->uusiVarmenneNappi, &QPushButton::clicked, this, &VeroMaaritys::lisaaVarmenne);
    connect( ui->poistaVarmenneNappi, &QPushButton::clicked, this, &VeroMaaritys::poistaVarmenne);
    connect( tila, &VeroVarmenneTila::paivitetty, this, &VeroMaaritys::tilaPaivitetty);

    connect( ui->maksuALVNappi, &QPushButton::clicked, this, &VeroMaaritys::maksuAlv);

    ui->kausiCombo->addItem(tr("Kuukausi"),1);
    ui->kausiCombo->addItem(tr("Neljännesvuosi"),3);
    ui->kausiCombo->addItem(tr("Vuosi"), 12);


    ui->puhelinEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\+\\d+")));
}

VeroMaaritys::~VeroMaaritys()
{
    delete ui;
}

bool VeroMaaritys::nollaa()
{
    ui->varmenneGroup->setVisible(false);

    if( kp()->pilvi()->pilvi())
        tila->paivita();

    paivitaMaksuAlvTieto();
    ui->kausiCombo->setCurrentIndex( ui->kausiCombo->findData( kp()->asetukset()->asetus(AsetusModel::AlvKausi) ) );


    return TallentavaMaaritysWidget::nollaa();
}

bool VeroMaaritys::onkoMuokattu()
{
    return TallentavaMaaritysWidget::onkoMuokattu() ||
            kp()->asetukset()->asetus(AsetusModel::AlvKausi) != ui->kausiCombo->currentData().toInt();
}

bool VeroMaaritys::tallenna()
{
    kp()->asetukset()->aseta(AsetusModel::AlvKausi, ui->kausiCombo->currentData().toInt());
    return TallentavaMaaritysWidget::tallenna();
}

void VeroMaaritys::tilaPaivitetty()
{
    ui->varmenneGroup->setVisible(true);
    ui->varmenneInfo->setText(tila->toString());

    ui->uusiVarmenneNappi->setVisible(
            tila->status() != "OK" &&
            tila->status() != "PG" &&
            !kp()->asetukset()->ytunnus().isEmpty());
    ui->poistaVarmenneNappi->setVisible( tila->status() == "OK");

    if( tila->isValid() && (ui->omaehtoistenviiteEdit->text().isEmpty() || ui->tuloveroviiteEdit->text().isEmpty()))
        haeViitteet();
}

void VeroMaaritys::haeViitteet()
{
    QString url = QString("%1/info/refs").arg(kp()->pilvi()->service("vero"));
    KpKysely* kysymys = kpk(url);
    connect(kysymys, &KpKysely::vastaus, this, &VeroMaaritys::viitteetSaapuu);
    kysymys->kysy();
}

void VeroMaaritys::viitteetSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    if(map.isEmpty()) return;
    if( ui->omaehtoistenviiteEdit->text().isEmpty())
        ui->omaehtoistenviiteEdit->setText( map.value("172").toString() );
    if( ui->tuloveroviiteEdit->text().isEmpty())
        ui->tuloveroviiteEdit->setText(map.value("173").toString());
    tarkastaMuokkaus();
}

void VeroMaaritys::lisaaVarmenne()
{
    VarmenneDialog dlg(this);
    if( dlg.exec() == QDialog::Accepted) {
        QVariantMap payload;
        payload.insert("businessid", kp()->asetukset()->ytunnus());
        payload.insert("name", kp()->asetukset()->nimi());
        payload.insert("transferid", dlg.tunnus());
        payload.insert("password", dlg.salasana());
        QString url = QString("%1/cert").arg(kp()->pilvi()->service("vero"));
        KpKysely* kysymys = kpk(url, KpKysely::PUT);
        connect( kysymys, &KpKysely::vastaus, tila, &VeroVarmenneTila::paivita);
        kysymys->kysy(payload);
        QTimer::singleShot(42000, tila, &VeroVarmenneTila::paivita);
    }
}

void VeroMaaritys::poistaVarmenne()
{
    if( QMessageBox::question(this, tr("Varmenteen poistaminen"),
                              tr("Haluatko todella poistaa verohallinnon varmenteen käytöstä?")) == QMessageBox::Yes) {
        QString url = QString("%1/cert").arg(kp()->pilvi()->service("vero"));
        KpKysely* kysymys = kpk(url, KpKysely::DELETE);
        connect( kysymys, &KpKysely::vastaus, tila, &VeroVarmenneTila::paivita);
        kysymys->kysy();
    }
}

void VeroMaaritys::paivitaMaksuAlvTieto()
{
    QDate alkaa = kp()->asetukset()->pvm("MaksuAlvAlkaa");
    QDate loppuu = kp()->asetukset()->pvm("MaksuAlvLoppuu");

    if( !alkaa.isValid())
        ui->maksuAlvInfo->setText(tr("Ei käytössä"));
    else if( !loppuu.isValid())
        ui->maksuAlvInfo->setText(tr("Käytössä %1 alkaen").arg(alkaa.toString("dd.MM.yyyy")));
    else
        ui->maksuAlvInfo->setText( tr("%1 - %2").arg(alkaa.toString("dd.MM.yyyy"), loppuu.toString("dd.MM.yyyy")));
}

void VeroMaaritys::maksuAlv()
{
    if( !kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).onkoValidi() || !kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).onkoValidi() )
    {
        QMessageBox::critical(nullptr, tr("Tilikartta puutteellinen"), tr("Maksuperusteiseen arvonlisäveroon tarvittavat kohdentamattoman arvonlisäverovelan ja/tai "
                                                                    "arvonlisäverosaatavien tilit puuttuvat.\n"
                                                                    "Ottaaksesi maksuperusteisen arvonlisäveron käyttöön lisää tarvittavat tilit "
                                                                    "tilikarttaasi"));
        return;
    }

    QDialog dlg;
    Ui::Maksuperusteinen ui;
    ui.setupUi(&dlg);

    QDate alkaa = kp()->asetukset()->pvm("MaksuAlvAlkaa");
    QDate loppuu = kp()->asetukset()->pvm("MaksuAlvLoppuu");

    // Oletuksena uudelle aloitukselle on seuraavan kuukauden alku
    if( !alkaa.isValid())
    {
        alkaa = kp()->paivamaara();
        alkaa = alkaa.addMonths(1);
        alkaa.setDate( alkaa.year(), alkaa.month(), 1 );
    }
    if( !loppuu.isValid())
    {
        loppuu.setDate( kp()->tilikaudet()->kirjanpitoLoppuu().year() + 1, 1, 1 );
    }

    ui.alkaaDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa());
    ui.paattyyDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa());

    ui.alkaaCheck->setChecked( kp()->asetukset()->onko("MaksuAlvAlkaa") );
    ui.alkaaDate->setEnabled( kp()->asetukset()->onko("MaksuAlvAlkaa") );
    ui.alkaaDate->setDate( alkaa );
    ui.paattyyCheck->setChecked( kp()->asetukset()->onko("MaksuAlvLoppuu"));
    ui.paattyyDate->setEnabled( kp()->asetukset()->onko("MaksuAlvLoppuu"));
    ui.paattyyDate->setDate( loppuu );

    connect( ui.ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("alv/maksuperusteinen");} );

    if( dlg.exec() == QDialog::Accepted)
    {
        if( ui.alkaaCheck->isChecked())
            kp()->asetukset()->aseta("MaksuAlvAlkaa", ui.alkaaDate->date());
        else
            kp()->asetukset()->poista("MaksuAlvAlkaa");

        if( ui.paattyyCheck->isChecked())
            kp()->asetukset()->aseta("MaksuAlvLoppuu", ui.paattyyDate->date());
        else
            kp()->asetukset()->poista("MaksuAlvLoppuu");
        paivitaMaksuAlvTieto();
    }
}
