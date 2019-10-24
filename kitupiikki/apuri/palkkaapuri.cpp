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
#include "palkkaapuri.h"
#include "ui_palkkaapuri.h"
#include "db/kirjanpito.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "model/tosite.h"

#include <QJsonDocument>
#include <QVariant>

PalkkaApuri::PalkkaApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget(parent, tosite),
    ui(new Ui::PalkkaApuri)
{
    ui->setupUi(this);

    for( KpEuroEdit *euro : findChildren<KpEuroEdit*>() )
        connect( euro, &KpEuroEdit::textChanged, this, &PalkkaApuri::tositteelle);
    connect( ui->tiliCombo, &TiliCombo::currentTextChanged, this, &PalkkaApuri::tositteelle);
    connect( tosite, &Tosite::otsikkoMuuttui, this, &PalkkaApuri::tositteelle);
    connect( tosite, &Tosite::pvmMuuttui, this, &PalkkaApuri::tositteelle);


    // Luetaan palkkakoodit
    palkkatilit_ = QJsonDocument::fromJson( kp()->asetus("palkkatilit").toUtf8() ).toVariant().toMap();
}

PalkkaApuri::~PalkkaApuri()
{
    delete ui;
}

void PalkkaApuri::teeReset()
{
    ui->tiliCombo->suodataTyypilla("ARP");

    QVariantList vientilista = tosite()->viennit()->viennit().toList();

    QMap<QString,double> eurot;

    for(QVariant item : vientilista) {
        TositeVienti vienti(item.toMap());

        eurot.insert(vienti.palkkakoodi(), vienti.debet() > 1e-5 ? vienti.debet() : vienti.kredit());
        if( vienti.palkkakoodi() == "MP" )
            ui->tiliCombo->valitseTili( vienti.tili() );

    }

    ui->palkkaEdit->setValue( eurot.value("PA") );
    ui->palkkioEdit->setValue(eurot.value("PI"));
    ui->lisaEdit->setValue(eurot.value("LK"));
    ui->lomaEdit->setValue(eurot.value("LS"));
    ui->luontoisEdit->setValue(eurot.value("LU"));
    ui->kmEdit->setValue(eurot.value("KM"));
    ui->paivarahaEdit->setValue(eurot.value("PR"));
    ui->ateriaEdit->setValue(eurot.value("AT"));
    ui->pidatysEdit->setValue(eurot.value("EP"));
    ui->elakeEdit->setValue(eurot.value("EL"));
    ui->tvredit->setValue(eurot.value("TV"));
    ui->ayedit->setValue(eurot.value("AY"));
    ui->svedit->setValue(eurot.value("SV"));
    ui->nettoLabel->setText(QString("%L1 €").arg(eurot.value("MP"),10,'f',2));

}

bool PalkkaApuri::teeTositteelle()
{
    QVariantList viennit;

    qlonglong brutto = qRound64( ui->palkkaEdit->value() * 100 ) +
            qRound64( ui->palkkioEdit->value() * 100) +
            qRound64( ui->lisaEdit->value() * 100) +
            qRound64( ui->lomaEdit->value() * 100);

    qlonglong vahennykset = qRound64( ui->pidatysEdit->value() * 100 ) +
            qRound64( ui->elakeEdit->value() * 100) +
            qRound64( ui->tvredit->value() * 100) +
            qRound64( ui->ayedit->value() * 100);

    qlonglong korvaukset = qRound64( ui->kmEdit->value() * 100) +
            qRound64( ui->paivarahaEdit->value() * 100) +
            qRound64( ui->ateriaEdit->value() * 100);

    double maksettavaa = (brutto + korvaukset - vahennykset) / 100.0;
    kirjaa( viennit, QString(), 0, maksettavaa  , "Maksetut palkat", "MP");
    ui->nettoLabel->setText(QString("%L1 €").arg(maksettavaa,10,'f',2));

    kirjaa( viennit, "PA", ui->palkkaEdit->value(), 0, "Palkat");
    kirjaa( viennit, "PI", ui->palkkioEdit->value(), 0, "Palkkiot");
    kirjaa( viennit, "LK", ui->lisaEdit->value(), 0, "Lisät");
    kirjaa( viennit, "LS", ui->lomaEdit->value(), 0, "Loma-ajan ja sos.palkat");


    kirjaa( viennit, "LU", ui->luontoisEdit->value(), 0, "Luontoisedut");
    kirjaa( viennit, "LV", 0, ui->luontoisEdit->value());

    kirjaa( viennit, "KM", ui->kmEdit->value(), 0, "Km-korvaukset");
    kirjaa( viennit, "PR", ui->paivarahaEdit->value(), 0, "Päivärahat");
    kirjaa( viennit, "AT", ui->ateriaEdit->value(), 0, "Ateriakorvaukset");

    kirjaa( viennit, "EP", 0, ui->pidatysEdit->value(), "Ennakonpidätys");
    kirjaa( viennit, "EL", 0, ui->elakeEdit->value(), "Työntekijöiden eläkevakuutusmaksut");
    kirjaa( viennit, "TV", 0, ui->tvredit->value(), "Työntekijöiden TVR-maksut");
    kirjaa( viennit, "AY", 0, ui->ayedit->value(), "Ay-jäsenmaksut");

    kirjaa( viennit, "SV", ui->svedit->value(), 0, "Sairausvakuutusmaksu");
    kirjaa( viennit, "SB", 0, ui->svedit->value(), "Sairausvakuutusmaksu");

    tosite()->viennit()->asetaViennit(viennit);

    return true;
}

void PalkkaApuri::kirjaa(QVariantList &lista, const QString &palkkakoodi, double debet, double kredit,
                         const QString& selite, const QString& tallennuskoodi)
{
    if( debet > 1e-5 || kredit > 1e-5) {
        TositeVienti vienti;
        vienti.setPvm( tosite()->pvm() );

        if( palkkakoodi.isNull())
            vienti.setTili( ui->tiliCombo->valittuTilinumero() );
        else
            vienti.setTili( palkkatilit_.value(palkkakoodi).toInt() );
        if( debet > 1e-5 )
            vienti.setDebet( debet );
        else if( kredit > 1e-5)
            vienti.setKredit( kredit );

        QString riviselite;
        if( !selite.isEmpty()) {
            riviselite = selite;
            if( !tosite()->otsikko().isEmpty())
                riviselite.append(" / ");
        }
        riviselite.append( tosite()->otsikko() );
        vienti.setSelite(riviselite);
        vienti.setPalkkakoodi( tallennuskoodi.isEmpty() ? palkkakoodi : tallennuskoodi );

        lista.append(vienti);
    }
}
