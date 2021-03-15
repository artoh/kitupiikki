/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#include <QSqlQuery>

#include "tilikarttaraportti.h"
#include "tilikarttalistaaja.h"
#include "db/asetusmodel.h"
#include <QJsonDocument>
#include "kieli/monikielinen.h"
#include "kieli/kielet.h"

TilikarttaRaportti::TilikarttaRaportti()
    : RaporttiWidget(nullptr)
{
    ui = new Ui::TilikarttaRaportti;
    ui->setupUi( raporttiWidget);

    connect( ui->tilikaudeltaCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaPaiva()));

    ui->tilikaudeltaCombo->setModel( kp()->tilikaudet() );
    ui->tilikaudeltaCombo->setModelColumn( TilikausiModel::KAUSI );
    ui->tilikaudeltaCombo->setCurrentIndex( ui->tilikaudeltaCombo->count() - 1);

    ui->saldotDate->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(),
                                  kp()->tilikaudet()->kirjanpitoLoppuu());


    // Haetaan combon vaihtoehdot
    ui->kieliCombo->clear();
    QVariantMap vemap = QJsonDocument::fromJson( kp()->asetukset()->asetus("kielet").toUtf8() ).toVariant().toMap();
    QMapIterator<QString,QVariant> viter(vemap);
    while( viter.hasNext()) {
        viter.next();
        Monikielinen kielet(viter.value());
        ui->kieliCombo->addItem(QIcon(":/liput/" + viter.key() + ".png"), kielet.teksti(), viter.key() );
    }

    int kieliIndeksi = ui->kieliCombo->findData( Kielet::instanssi()->nykyinen() );
    if( kieliIndeksi > -1)
        ui->kieliCombo->setCurrentIndex( kieliIndeksi );

}

TilikarttaRaportti::~TilikarttaRaportti()
{
    delete ui;
}

void TilikarttaRaportti::esikatsele()
{
    TiliKarttaListaaja *listaaja = new TiliKarttaListaaja(this);
    connect( listaaja, &TiliKarttaListaaja::valmis,
             this, &RaporttiWidget::nayta);

    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( ui->tilikaudeltaCombo->currentData( TilikausiModel::PaattyyRooli ).toDate() );

    TiliKarttaListaaja::KarttaValinta valinta = TiliKarttaListaaja::KAIKKI_TILIT;
    if( ui->kaytossaRadio->isChecked())
        valinta = TiliKarttaListaaja::KAYTOSSA_TILIT;
    else if( ui->kirjauksiaRadio->isChecked())
        valinta = TiliKarttaListaaja::KIRJATUT_TILIT;
    else if( ui->suosikkiRadio->isChecked())
        valinta = TiliKarttaListaaja::SUOSIKKI_TILIT;

    QDate saldopaiva;
    if( ui->saldotCheck->isChecked())
        saldopaiva = ui->saldotDate->date();


    listaaja->kirjoita( valinta, kausi, ui->otsikotCheck->isChecked(),
                        ui->tilityypitCheck->isChecked(), saldopaiva,
                        ui->kirjausohjeet->isChecked(),
                        ui->kieliCombo->currentData().toString());
}

void TilikarttaRaportti::paivitaPaiva()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( ui->tilikaudeltaCombo->currentData( TilikausiModel::PaattyyRooli ).toDate() );
    ui->saldotDate->setDate( kausi.paattyy());
}
