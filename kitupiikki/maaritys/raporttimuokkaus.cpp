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

#include "raporttimuokkaus.h"
#include "db/kirjanpito.h"
#include "raportinkorostin.h"

#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>

RaporttiMuokkaus::RaporttiMuokkaus(QWidget *parent) :
    MaaritysWidget(parent)
{
    ui = new Ui::RaporttiMuokkain;
    ui->setupUi(this);

    new RaportinKorostin( ui->editori->document() );

    connect( ui->valintaCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(avaaRaportti(QString)));
    connect( ui->editori, SIGNAL(textChanged()), this, SLOT(merkkaaMuokattu()));
    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusi()));
    connect( ui->kopioiNappi, SIGNAL(clicked(bool)), this, SLOT(kopio()));
    connect( ui->nimeaNappi, SIGNAL(clicked(bool)), this, SLOT(nimea()));
    connect( ui->tyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(merkkaaMuokattu()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poista()));

    connect( ui->arkistoBox, &QCheckBox::clicked, [this] (bool laitetaanko) { this->raportinArkistointi(laitetaanko,false); } );
    connect( ui->vertailuCheck, &QCheckBox::clicked, [this] (bool laitetaanko) { this->raportinArkistointi(laitetaanko,true); } );


    connect( ui->tyyppiCombo, &QComboBox::currentTextChanged, [this] { this->ui->vertailuCheck->setEnabled( ui->tyyppiCombo->currentData().toString() != ":tase"); } );

    ui->tyyppiCombo->addItem( tr("Tuloslaskelma"), ":tulos");
    ui->tyyppiCombo->addItem( tr("Tase"), ":tase");
    ui->tyyppiCombo->addItem( tr("Kohdennuslaskelma"), ":kohdennus");




    muokattu = false;
}

RaporttiMuokkaus::~RaporttiMuokkaus()
{
    delete ui;
}

bool RaporttiMuokkaus::nollaa()
{
    // Haetaan comboon käytettävissä olevat raportit
    ui->valintaCombo->clear();

    QStringList raportit = kp()->asetukset()->avaimet("Raportti/");
    raportit.sort(Qt::CaseInsensitive);

    foreach (QString nimi, raportit )
    {
        ui->valintaCombo->addItem( nimi.mid(9) );
    }

    return true;
}

bool RaporttiMuokkaus::tallenna()
{

    QString tallennettava = ui->tyyppiCombo->currentData().toString();
    tallennettava.append("\n");
    tallennettava.append( ui->editori->toPlainText());

    kp()->asetukset()->aseta( "Raportti/" + nimi, tallennettava);

    muokattu = false;
    emit tallennaKaytossa(false);

    return true;
}

bool RaporttiMuokkaus::onkoMuokattu()
{
    return muokattu;
}

void RaporttiMuokkaus::avaaRaportti(const QString &raportti)
{
    if( onkoMuokattu() && raportti != nimi && ui->editori->toPlainText().length() > 10)
        if( !kysyTallennus())
        {
            ui->valintaCombo->setCurrentIndex( ui->valintaCombo->findText( nimi) );
            return;
        }

    if( !raportti.isEmpty() && nimi != raportti)
    {

        QString teksti = kp()->asetukset()->asetus( "Raportti/" + raportti );
        int rivinvaihto = teksti.indexOf('\n');

        QString ekarivi = teksti.left( rivinvaihto );
        ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData(ekarivi));

        ui->editori->setPlainText(teksti.mid(rivinvaihto + 1));
        muokattu = false;
        nimi = raportti;
        emit tallennaKaytossa(false);

         ui->arkistoBox->setChecked(kp()->asetukset()->lista("ArkistoRaportit").contains(raportti) );
         ui->vertailuCheck->setChecked(kp()->asetukset()->lista("ArkistoRaportit").contains(raportti + QString("$")) );
    }
}

void RaporttiMuokkaus::merkkaaMuokattu()
{
    muokattu = true;
    emit tallennaKaytossa(true);
}

void RaporttiMuokkaus::uusi()
{
    if( aloitaUusi() )
        ui->editori->clear();
}

void RaporttiMuokkaus::kopio()
{
    aloitaUusi();
}

void RaporttiMuokkaus::nimea()
{
    QString uusinimi  = QInputDialog::getText(this, tr("Nimeä raportti"), tr("Raportin uusi nimi"), QLineEdit::Normal, nimi);
    if( !uusinimi.isEmpty())
    {
        if( !kp()->asetukset()->asetus("Raportti/" + uusinimi).isEmpty())
            if( QMessageBox::question( this, tr("Raportti"), tr("Raportti %1 on jo olemassa. Korvataanko raportti?").arg(uusinimi),
                                       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
                return;

        // Vaihdetaan uuteen nimeen
        kp()->asetukset()->aseta("Raportti/" + uusinimi, kp()->asetukset()->asetus("Raportti/" + nimi));
        kp()->asetukset()->poista("Raportti/" + nimi);

        nimi = uusinimi;
        // Vaihdetaan nimi valintaboksiin
        ui->valintaCombo->setItemText( ui->valintaCombo->currentIndex(), nimi);
    }
}

void RaporttiMuokkaus::poista()
{
    if( QMessageBox::question(this, tr("Raportin poistaminen"), tr("Poistetaanko tämä raportti?\nValintaa ei voi perua!"),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
    {
        muokattu = false;
        kp()->asetukset()->poista("Raportti/" + nimi);
        ui->editori->clear();
        ui->valintaCombo->removeItem( ui->valintaCombo->currentIndex());
    }
}

bool RaporttiMuokkaus::kysyTallennus()
{
    int tallennetaanko =  QMessageBox::question(this, tr("Raportin muokkaus"),
                                                tr("Raporttia on muokattu.\n"
                                                   "Tallennetaanko muokattu raportti?"),
                                                QMessageBox::Save | QMessageBox::No | QMessageBox::Cancel);
    if( tallennetaanko == QMessageBox::Save)
        tallenna();
    return tallennetaanko == QMessageBox::Save || tallennetaanko == QMessageBox::No;
}

bool RaporttiMuokkaus::aloitaUusi()
{
    if( onkoMuokattu()  && ui->editori->toPlainText().length() > 10 )
        if( !kysyTallennus())
            return false;

    QString uusinimi  = QInputDialog::getText(this, tr("Uusi raportti"), tr("Uuden raportin nimi"), QLineEdit::Normal );
    if( !uusinimi.isEmpty())
    {
        if( !kp()->asetukset()->asetus("Raportti/" + uusinimi).isEmpty())
            if( QMessageBox::question( this, tr("Raportti"), tr("Raportti %1 on jo olemassa. Korvataanko raportti?").arg(uusinimi),
                                       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
                return false;

        nimi = uusinimi;
        ui->valintaCombo->addItem( nimi );
        ui->valintaCombo->setCurrentIndex( ui->valintaCombo->count() - 1);  // Valitaan tämä viimeisin
        muokattu = false;
    }
    return true;

}

void RaporttiMuokkaus::raportinArkistointi(bool laitetaanko, bool vertailu)
{
    QStringList raporttilista = kp()->asetukset()->lista("ArkistoRaportit");
    QString raporttinimi = vertailu ? nimi + QString("$") : nimi ;

    if( laitetaanko )
    {
        if( !raporttilista.contains(raporttinimi))
            raporttilista.append(raporttinimi);
    }
    else
        raporttilista.removeAll(raporttinimi);

    raporttilista.sort(Qt::CaseInsensitive);
    kp()->asetukset()->aseta("ArkistoRaportit", raporttilista);
    merkkaaMuokattu();
}

