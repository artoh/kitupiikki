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
#include "model/euro.h"

#include <QJsonDocument>
#include <QVariant>

PalkkaApuri::PalkkaApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget(parent, tosite),
    ui(new Ui::PalkkaApuri)
{
    ui->setupUi(this);

    for( KpEuroEdit *euro : findChildren<KpEuroEdit*>() )
        connect( euro, &KpEuroEdit::sntMuuttui, this, &PalkkaApuri::tositteelle);
    connect( ui->tiliCombo, &TiliCombo::currentTextChanged, this, &PalkkaApuri::tositteelle);
    connect( tosite, &Tosite::otsikkoMuuttui, this, &PalkkaApuri::tositteelle);
    connect( tosite, &Tosite::pvmMuuttui, this, &PalkkaApuri::tositteelle);
    connect( ui->kohdennusCombo, &KohdennusCombo::currentTextChanged, this, &PalkkaApuri::tositteelle);
    connect( tosite, &Tosite::pvmMuuttui, ui->kohdennusCombo, &KohdennusCombo::suodataPaivalla);

    // Luetaan palkkakoodit
    palkkatilit_ = QJsonDocument::fromJson( kp()->asetukset()->asetus(AsetusModel::Palkkatilit).toUtf8() ).toVariant().toMap();
}

PalkkaApuri::~PalkkaApuri()
{
    delete ui;
}

void PalkkaApuri::otaFokus()
{
    ui->palkkaEdit->setFocus();
}

void PalkkaApuri::teeReset()
{
    ui->tiliCombo->suodataTyypilla("(ARP|BS)");
    int vastatili = kp()->asetukset()->luku("PalkkaVastatili") ? kp()->asetukset()->luku("PalkkaVastatili") : kp()->tilit()->tiliTyypilla(TiliLaji::PANKKITILI).numero();
    ui->tiliCombo->valitseTili( vastatili  );


    QMap<QString,Euro> eurot;

    for(const auto& vienti : tosite()->viennit()->viennit()) {


        eurot.insert(vienti.palkkakoodi(), vienti.debetEuro() ? vienti.debetEuro() : vienti.kreditEuro());
        if( vienti.palkkakoodi() == "MP" )
            ui->tiliCombo->valitseTili( vienti.tili() );
        if( vienti.kohdennus() != 0)
            ui->kohdennusCombo->valitseKohdennus(vienti.kohdennus());

    }

    ui->palkkaEdit->setEuro(eurot.value("PA") );
    ui->palkkioEdit->setEuro(eurot.value("PI"));
    ui->lisaEdit->setEuro(eurot.value("LK"));
    ui->lomaEdit->setEuro(eurot.value("LS"));
    ui->luontoisEdit->setEuro(eurot.value("LU"));
    ui->kmEdit->setEuro(eurot.value("KM"));
    ui->paivarahaEdit->setEuro(eurot.value("PR"));
    ui->ateriaEdit->setEuro(eurot.value("AT"));
    ui->pidatysEdit->setEuro(eurot.value("EP"));
    ui->elakeEdit->setEuro(eurot.value("EL"));
    ui->tvredit->setEuro(eurot.value("TV"));
    ui->ayedit->setEuro(eurot.value("AY"));
    ui->uoedit->setEuro(eurot.value("UO"));
    ui->svedit->setEuro(eurot.value("SV"));
    ui->nettoLabel->setText( eurot.value("MP").display());

    ui->kohdennusCombo->suodataPaivalla( tosite()->pvm() );
    ui->kohdennusLabel->setVisible( kp()->kohdennukset()->kohdennuksia() );
    ui->kohdennusCombo->setVisible( kp()->kohdennukset()->kohdennuksia());

}

bool PalkkaApuri::teeTositteelle()
{
    QVariantList viennit;

    Euro brutto = ui->palkkaEdit->euro() +
            ui->palkkioEdit->euro() +
            ui->lisaEdit->euro() +
            ui->lomaEdit->euro();

    Euro vahennykset = ui->pidatysEdit->euro() +
            ui->elakeEdit->euro() +
            ui->tvredit->euro() +
            ui->ayedit->euro() +
            ui->uoedit->euro();

    Euro korvaukset = ui->kmEdit->euro() +
            ui->paivarahaEdit->euro() +
            ui->ateriaEdit->euro();


    Euro maksettavaa = brutto + korvaukset - vahennykset;
    kirjaa( viennit, QString(), 0, maksettavaa  , "Palkat", "MP");
    ui->nettoLabel->setText(maksettavaa.display());

    kirjaa( viennit, "PA", ui->palkkaEdit->euro(), 0, "Palkat");
    kirjaa( viennit, "PI", ui->palkkioEdit->euro(), 0, "Palkkiot");
    kirjaa( viennit, "LK", ui->lisaEdit->euro(), 0, "Lisät");
    kirjaa( viennit, "LS", ui->lomaEdit->euro(), 0, "Loma-ajan ja sos.palkat");


    kirjaa( viennit, "LU", ui->luontoisEdit->euro(), 0, "Luontoisedut");
    kirjaa( viennit, "LV", 0, ui->luontoisEdit->euro());

    kirjaa( viennit, "KM", ui->kmEdit->euro(), 0, "Km-korvaukset");
    kirjaa( viennit, "PR", ui->paivarahaEdit->euro(), 0, "Päivärahat");
    kirjaa( viennit, "AT", ui->ateriaEdit->euro(), 0, "Ateriakorvaukset");

    kirjaa( viennit, "EP", 0, ui->pidatysEdit->euro(), "Ennakonpidätys");
    kirjaa( viennit, "EL", 0, ui->elakeEdit->euro(), "Työntekijöiden eläkevakuutusmaksut");
    kirjaa( viennit, "TV", 0, ui->tvredit->euro(), "Työntekijöiden TVR-maksut");
    kirjaa( viennit, "AY", 0, ui->ayedit->euro(), "Ay-jäsenmaksut");
    kirjaa( viennit, "UO", 0, ui->uoedit->euro(), "Ulosottotilitykset");

    kirjaa( viennit, "SV", ui->svedit->euro(), 0, "Sairausvakuutusmaksu");
    kirjaa( viennit, "SB", 0, ui->svedit->euro(), "Sairausvakuutusmaksu");

    tosite()->viennit()->asetaViennit(viennit);

    kp()->asetukset()->aseta("PalkkaVastatili", ui->tiliCombo->valittuTilinumero());

    return true;
}

void PalkkaApuri::kirjaa(QVariantList &lista, const QString &palkkakoodi, const Euro& debet, const Euro& kredit,
                         const QString& selite, const QString& tallennuskoodi)
{
    if( debet || kredit ) {
        TositeVienti vienti;
        vienti.setPvm( tosite()->pvm() );

        if( palkkakoodi.isNull())
            vienti.setTili( ui->tiliCombo->valittuTilinumero() );
        else
            vienti.setTili( palkkatilit_.value(palkkakoodi).toInt() );
        if( debet )
            vienti.setDebet( debet );
        else if( kredit)
            vienti.setKredit( kredit );

        if( kp()->tilit()->tiliNumerolla(vienti.tili()).tyyppi().onko(TiliLaji::TULOS))
            vienti.setKohdennus(ui->kohdennusCombo->kohdennus());

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
