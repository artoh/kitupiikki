/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "tasetulosraportti.h"
#include "db/kirjanpito.h"
#include "kieli/kielet.h"

#include <QJsonDocument>
#include <QStringListModel>
#include <QDebug>
#include <QMessageBox>

TaseTulosRaportti::TaseTulosRaportti(const QString &raportinTyyppi, bool kuukausittain, QWidget *parent) :
    RaporttiWidget (parent),
    ui( new Ui::MuokattavaRaportti ),
    tyyppi_(raportinTyyppi),
    kuukausittain_(kuukausittain)

{
    ui->setupUi( raporttiWidget );

    paivitaMuodot();
    paivitaKielet();
    muotoVaihdettu();
    kieliVaihdettu();

    connect( ui->muotoCombo, &QComboBox::currentTextChanged, this, &TaseTulosRaportti::muotoVaihdettu);
    connect( ui->kieliCombo, &QComboBox::currentTextChanged, this, &TaseTulosRaportti::kieliVaihdettu);

    if( tyyppi_ == "projektit") {
        ui->kohdennusCheck->setVisible( kp()->kohdennukset()->kustannuspaikkoja() );
        ui->kohdennusCombo->setVisible( kp()->kohdennukset()->kustannuspaikkoja());
        ui->kohdennusCheck->setText( tr("Kustannuspaikalla"));
        ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KUSTANNUSPAIKAT);
    }
    else if( tyyppi_ == "tase" || tyyppi_ == "kohdennus" || ( !kp()->kohdennukset()->kohdennuksia() && !kp()->kohdennukset()->merkkauksia() ))
    {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    } else {
        ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KAIKKI);
    }

    QStringList tyyppiLista;
    tyyppiLista << tr("Toteutunut") << tr("Budjetti") << tr("Budjettiero €") << tr("Toteutunut %");
    QStringListModel *tyyppiListaModel = new QStringListModel(this);
    tyyppiListaModel->setStringList(tyyppiLista);

    ui->tyyppi1->setModel(tyyppiListaModel);
    ui->tyyppi2->setModel(tyyppiListaModel);
    ui->tyyppi3->setModel(tyyppiListaModel);
    ui->tyyppi4->setModel(tyyppiListaModel);

    // Jos alkupäivämäärä on tilikauden aloittava, päivitetään myös päättymispäivä tilikauden päättäväksi
    connect( ui->alkaa1Date, &KpDateEdit::dateChanged, this, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu1Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });
    connect( ui->alkaa2Date, &KpDateEdit::dateChanged, this, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu2Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });
    connect( ui->alkaa3Date, &KpDateEdit::dateChanged, this, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });
    connect( ui->alkaa4Date, &KpDateEdit::dateChanged, this, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu4Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });

    for(QObject* widget : ui->pvmKehys->children()) {
        KpDateEdit* date = qobject_cast<KpDateEdit*>(widget);
        if(date)
            connect(date, &KpDateEdit::dateChanged, this, &TaseTulosRaportti::paivitaKohdennusPaivat);
        else {
            QCheckBox* box = qobject_cast<QCheckBox*>(widget);
            if(box)
                connect(box, &QCheckBox::toggled, this, &TaseTulosRaportti::paivitaKohdennusPaivat);
        }
    }

    paivitaUi();
}


void TaseTulosRaportti::tallenna()
{
    aseta(RaporttiValinnat::Tyyppi, tyyppi_);
    aseta(RaporttiValinnat::Kuukausittain, kuukausittain_);
    aseta(RaporttiValinnat::RaportinMuoto, ui->muotoCombo->currentData().toString());
    aseta(RaporttiValinnat::Kieli, ui->kieliCombo->currentData().toString());
    aseta(RaporttiValinnat::TulostaErittely, ui->erittelyCheck->isChecked());

    aseta(RaporttiValinnat::AlkuPvm, ui->alkuEdit->date());
    aseta(RaporttiValinnat::LoppuPvm, ui->loppuEdit->date());
    aseta(RaporttiValinnat::KuukaudetYhteensa, ui->summaCheck->isChecked());
    aseta(RaporttiValinnat::Jaksotettu, ui->jaksotusCheck->isChecked());

    aseta(RaporttiValinnat::Kohdennuksella, ui->kohdennusCheck->isVisible() && ui->kohdennusCheck->isChecked() && !ui->kohdennusCombo->currentText().isEmpty() ? ui->kohdennusCombo->kohdennus() : -1);

    kp()->raporttiValinnat()->tyhjennaSarakkeet();
    lisaaSarake(true, ui->alkaa1Date->date(), ui->loppuu1Date->date(), ui->tyyppi1->currentIndex());
    lisaaSarake(ui->sarake2Box->isChecked(), ui->alkaa2Date->date(), ui->loppuu2Date->date(), ui->tyyppi2->currentIndex());
    lisaaSarake(ui->sarake3Box->isChecked(), ui->alkaa3Date->date(), ui->loppuu3Date->date(), ui->tyyppi3->currentIndex());
    lisaaSarake(ui->sarake4Box->isChecked(), ui->alkaa4Date->date(), ui->loppuu4Date->date(), ui->tyyppi4->currentIndex());


}

void TaseTulosRaportti::muotoVaihtui()
{
    QString raportti = ui->muotoCombo->currentData().toString();
    QString kaava = kp()->asetukset()->asetus(raportti);
    if( kaava.isEmpty())
        return;
    kaava_ = kaava;
}

void TaseTulosRaportti::paivitaKielet()
{           
    QString nykykieli = ui->kieliCombo->currentData().toString();
    if( nykykieli.isEmpty())
        nykykieli = arvo(RaporttiValinnat::Kieli).toString();
    if( nykykieli.isEmpty())
        nykykieli = Kielet::instanssi()->nykyinen();

    QJsonDocument doc = QJsonDocument::fromJson( kaava_.toUtf8() );

    QVariantMap kielet = doc.toVariant().toMap().value("nimi").toMap();
    ui->kieliCombo->clear();

    for(const auto& kieli : kielet.keys()) {
        ui->kieliCombo->addItem( QIcon(":/liput/" + kieli + ".png"), kp()->asetukset()->kieli(kieli), kieli );
    }
    int kieliIndeksi = ui->kieliCombo->findData( nykykieli );
    if( kieliIndeksi > -1)
        ui->kieliCombo->setCurrentIndex( kieliIndeksi );
}

void TaseTulosRaportti::paivitaMuodot()
{
    paivitetaan_ = true;

    QStringList muodot;
    if(tyyppi_ == "tase" )
        muodot = kp()->asetukset()->avaimet("tase/");
    else
        muodot = kp()->asetukset()->avaimet("tulos/");    

    QString nykymuoto = ui->muotoCombo->currentData().toString();
    if( nykymuoto.isEmpty())
        nykymuoto=tyyppi_ == "tase" ? "tase/yleinen" : "tulos/yleinen";

    muotoVaihtui();

    const QString kieli = ui->kieliCombo->currentData().toString();

    ui->muotoCombo->clear();

    for( const auto& muoto : qAsConst( muodot )) {
        QString kaava = kp()->asetukset()->asetus(muoto);
        QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8() );
        QVariantMap map = doc.toVariant().toMap().value("muoto").toMap();
        QString muotonimi = map.value( kieli ).toString();
        ui->muotoCombo->addItem( muotonimi, muoto );
    }

    ui->muotoCombo->setCurrentIndex( ui->muotoCombo->findData(nykymuoto) );
    paivitetaan_ = false;
}

void TaseTulosRaportti::paivitaKohdennusPaivat()
{
    QDate pienin = ui->alkaa1Date->date();
    QDate isoin = ui->loppuu1Date->date();

    if( ui->sarake2Box->isChecked() && ui->alkaa2Date->date() < pienin) pienin = ui->alkaa2Date->date();
    if( ui->sarake3Box->isChecked() && ui->alkaa3Date->date() < pienin) pienin = ui->alkaa3Date->date();
    if( ui->sarake4Box->isChecked() && ui->alkaa4Date->date() < pienin) pienin = ui->alkaa4Date->date();
    if( ui->sarake2Box->isChecked() && ui->loppuu2Date->date() > isoin) isoin = ui->loppuu2Date->date();
    if( ui->sarake3Box->isChecked() && ui->loppuu3Date->date() > isoin) isoin = ui->loppuu3Date->date();
    if( ui->sarake4Box->isChecked() && ui->loppuu4Date->date() > isoin) isoin = ui->loppuu4Date->date();

    ui->kohdennusCombo->suodataValilla(pienin, isoin);
}

void TaseTulosRaportti::muotoVaihdettu()
{
    if(paivitetaan_) return;

    // Kielet päivitettävä valitulle muodolle
    paivitetaan_ = true;

    muotoVaihtui();
    paivitaKielet();

    paivitetaan_ = false;
}

void TaseTulosRaportti::kieliVaihdettu()
{
    if(paivitetaan_) return;
    paivitetaan_ = true;

    // Muodot vaihdettava toiselle kielelle
    paivitaMuodot();

    paivitetaan_ = false;
}

void TaseTulosRaportti::paivitaUi()
{
    // Jos tehdään Raportoija::TASElaskelmaa, piilotetaan turhat tiedot!

    bool kaudet = tyyppi_ != "tase";

    ui->alkaa1Date->setVisible( kaudet );
    ui->alkaa2Date->setVisible( kaudet );
    ui->alkaa3Date->setVisible( kaudet );
    ui->alkaa4Date->setVisible( kaudet );
    ui->alkaaLabel->setVisible( kaudet );
    ui->paattyyLabel->setVisible( kaudet );

    ui->tyyppi1->setVisible( kaudet );
    ui->tyyppi2->setVisible( kaudet );
    ui->tyyppi3->setVisible( kaudet );
    ui->tyyppi4->setVisible( kaudet );


    // Sitten laitetaan valmiiksi tilikausia nykyisestä taaksepäin
    int tilikausiIndeksi = kp()->tilikaudet()->indeksiPaivalle( kp()->paivamaara() );
    // #160 Tai sitten viimeinen tilikausi
    if( tilikausiIndeksi < 0)
        tilikausiIndeksi = kp()->tilikaudet()->rowCount(QModelIndex()) - 1;

    if( tilikausiIndeksi > -1 )
    {
        ui->alkaa1Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu1Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }
    if( tilikausiIndeksi > 0)
    {
        ui->alkaa2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-1).alkaa() );
        ui->loppuu2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-1).paattyy() );
    }
    else if( tilikausiIndeksi > -1)
    {
        ui->alkaa2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }


    ui->sarake2Box->setChecked(tilikausiIndeksi > 0);

    if( tilikausiIndeksi > 1)
    {
        ui->alkaa3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).alkaa() );
        ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).paattyy() );
    }
    else if( tilikausiIndeksi > -1)
    {
        ui->alkaa3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }


    ui->sarake3Box->setChecked(tilikausiIndeksi > 1);

    // Neljäs sarake oletuksena poissa käytöstä
    // Sitä tarvitaan lähinnä budjettivertailuihin
    if( tilikausiIndeksi > -1)
    {
        ui->alkaa4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }
    ui->sarake4Box->setChecked(false);

    ui->alkuEdit->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::AlkuPvm).toDate() );
    ui->loppuEdit->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::LoppuPvm).toDate() );
    ui->summaCheck->setChecked( kp()->raporttiValinnat()->arvo(RaporttiValinnat::KuukaudetYhteensa).toBool() );
    ui->jaksotusCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::Jaksotettu) );

    ui->pvmKehys->setVisible( !kuukausittain_ );
    ui->alkuLabel->setVisible( kuukausittain_ );
    ui->alkuEdit->setVisible( kuukausittain_ );
    ui->loppuLabel->setVisible( kuukausittain_ );
    ui->loppuEdit->setVisible( kuukausittain_ );
    ui->summaCheck->setVisible( kuukausittain_ && tyyppi_ == "tulos");
    ui->jaksotusCheck->setVisible( kuukausittain_ );

}

void TaseTulosRaportti::lisaaSarake(bool kaytossa, const QDate &alku, const QDate &loppu, int valintaIndeksi)
{
    if( kaytossa ) {
        RaporttiValintaSarake::SarakeTyyppi sarakeTyyppi = RaporttiValintaSarake::Toteutunut;
        if( valintaIndeksi == RaporttiValintaSarake::Budjetti)
            sarakeTyyppi = RaporttiValintaSarake::Budjetti;
        else if(valintaIndeksi == RaporttiValintaSarake::BudjettiEro)
            sarakeTyyppi = RaporttiValintaSarake::BudjettiEro;
        else if(valintaIndeksi == RaporttiValintaSarake::ToteumaProsentti)
            sarakeTyyppi = RaporttiValintaSarake::ToteumaProsentti;
        kp()->raporttiValinnat()->lisaaSarake(RaporttiValintaSarake(alku, loppu, sarakeTyyppi));
    }
}
