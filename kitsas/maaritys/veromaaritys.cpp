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

VeroMaaritys::VeroMaaritys() :
    ui{new Ui::VeroMaaritys},
    tila(new VeroVarmenneTila(this))
{
    ui->setupUi(this);

    connect( ui->uusiVarmenneNappi, &QPushButton::clicked, this, &VeroMaaritys::lisaaVarmenne);
    connect( ui->poistaVarmenneNappi, &QPushButton::clicked, this, &VeroMaaritys::poistaVarmenne);
    connect( tila, &VeroVarmenneTila::paivitetty, this, &VeroMaaritys::tilaPaivitetty);

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

    return TallentavaMaaritysWidget::nollaa();
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
