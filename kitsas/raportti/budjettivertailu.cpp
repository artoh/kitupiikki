/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "budjettivertailu.h"
#include "db/kirjanpito.h"
#include "db/tilikausimodel.h"
#include <QComboBox>
#include <QJsonDocument>
#include "kieli/kielet.h"

Budjettivertailu::Budjettivertailu() :
    RaporttiWidget(nullptr)
{
    ui = new Ui::Budjettivertailu;
    ui->setupUi(raporttiWidget);

    ui->kausiCombo->setModel(kp()->tilikaudet());
    ui->kausiCombo->setModelColumn(TilikausiModel::KAUSI);
    lataa();

    if( kp()->kohdennukset()->kohdennuksia())
    {
        ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);
        ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KOHDENNUKSET_PROJEKTIT);
        connect(ui->kausiCombo, &QComboBox::currentTextChanged, this, &Budjettivertailu::paivitaKausi);
    }
    else
    {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    }

    paivitaMuodot();
    paivitaKielet();
    paivitaKausi();

}

Budjettivertailu::~Budjettivertailu()
{
    delete ui;
}


void Budjettivertailu::paivitaMuodot()
{
    QStringList muodot = kp()->asetukset()->avaimet("tulos/");

    ui->muotoCombo->clear();
    int nykyinen = ui->muotoCombo->currentIndex() > -1 ? ui->muotoCombo->currentIndex() : 0;    

    for( const auto& muoto : qAsConst( muodot )) {
        QString kaava = kp()->asetukset()->asetus(muoto);
        QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8() );        
        Monikielinen muotoKielinen(doc.toVariant().toMap().value("muoto"));
        ui->muotoCombo->addItem( muotoKielinen.kaannos(ui->kieliCombo->kieli()), muoto );
    }

    ui->muotoCombo->setCurrentIndex(nykyinen);


}

void Budjettivertailu::paivitaKielet()
{
    QString raportti = ui->muotoCombo->currentData().toString();
    QString kaava = kp()->asetukset()->asetus(raportti);

    if( kaava == kaava_ || kaava.isEmpty())
        return;
    kaava_ = kaava;

    QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8() );

    QVariantMap kielet = doc.toVariant().toMap().value("nimi").toMap();
    ui->kieliCombo->clear();

    for(const auto& kieli : kielet.keys()) {
        ui->kieliCombo->addItem( lippu(kieli), kp()->asetukset()->kieli(kieli), kieli );
    }
    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( Kielet::instanssi()->nykyinen() ) );
}

void Budjettivertailu::paivitaKausi()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->kausiCombo->currentIndex() );
    ui->kohdennusCombo->suodataValilla(kausi.alkaa(), kausi.paattyy());
}

void Budjettivertailu::lataa()
{
    ui->kausiCombo->setCurrentIndex( kp()->tilikaudet()->indeksiPaivalle( arvo(RaporttiValinnat::AlkuPvm).toDate() ) );
    const int kohdennus = arvo(RaporttiValinnat::Kohdennuksella).toInt();
    ui->kohdennusCheck->setChecked( kohdennus > -1 );
    ui->kohdennusCombo->valitseKohdennus( kohdennus > -1 ? kohdennus : 0 );
    ui->erittelyCheck->setChecked( arvo(RaporttiValinnat::TulostaErittely).toBool());
}

void Budjettivertailu::tallenna()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->kausiCombo->currentIndex() );
    aseta(RaporttiValinnat::AlkuPvm, kausi.alkaa());
    aseta(RaporttiValinnat::LoppuPvm, kausi.paattyy());
    aseta(RaporttiValinnat::Kohdennuksella, ui->kohdennusCheck->isChecked() ? ui->kohdennusCombo->currentData().toInt() :  -1);
    aseta(RaporttiValinnat::Tyyppi, "tulos");
    aseta(RaporttiValinnat::RaportinMuoto, ui->muotoCombo->currentData().toString());
    aseta(RaporttiValinnat::TulostaErittely, ui->erittelyCheck->isChecked());
    aseta(RaporttiValinnat::Kieli, ui->kieliCombo->currentData().toString());

    kp()->raporttiValinnat()->tyhjennaSarakkeet();
    kp()->raporttiValinnat()->lisaaSarake(RaporttiValintaSarake(kausi.alkaa(), kausi.paattyy(), RaporttiValintaSarake::Budjetti));
    kp()->raporttiValinnat()->lisaaSarake(RaporttiValintaSarake(kausi.alkaa(), kausi.paattyy(), RaporttiValintaSarake::Toteutunut));
    kp()->raporttiValinnat()->lisaaSarake(RaporttiValintaSarake(kausi.alkaa(), kausi.paattyy(), RaporttiValintaSarake::BudjettiEro));
    kp()->raporttiValinnat()->lisaaSarake(RaporttiValintaSarake(kausi.alkaa(), kausi.paattyy(), RaporttiValintaSarake::ToteumaProsentti));

}
