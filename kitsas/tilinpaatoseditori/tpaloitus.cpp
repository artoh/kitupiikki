/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QStandardItem>

#include <QFileDialog>
#include <QFile>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QMessageBox>

#include "tpaloitus.h"
#include "ui_tpaloitus.h"

#include "db/kirjanpito.h"

#include <QDebug>
#include <QDesktopServices>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QVariant>

TpAloitus::TpAloitus(Tilikausi kausi, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TpAloitus),
    tilikausi(kausi),
    model(nullptr)
{
    ui->setupUi(this);

    int kaudenhenkilosto = kausi.henkilosto();
    int edellisenhenkilosto = kp()->tilikaudet()->tilikausiPaivalle(kausi.alkaa().addDays(-1)).henkilosto();

    ui->henkilostoSpin->setValue( kaudenhenkilosto ? kaudenhenkilosto : edellisenhenkilosto );

    connect( model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(valintaMuuttui(QStandardItem*)));
    connect( ui->lataaNappi, SIGNAL(clicked(bool)), this, SLOT(lataaTiedosto()));
    connect( ui->ohjeNappi, SIGNAL(clicked(bool)), this, SLOT(ohje()));
    connect( ui->henkilostoSpin, SIGNAL(valueChanged(int)), this, SLOT(tarkistaPMA()));

    connect( ui->mikroRadio, SIGNAL(clicked(bool)), this, SLOT(lataa()));
    connect( ui->pienRadio, SIGNAL(clicked(bool)), this, SLOT(lataa()));
    connect(ui->taysRadio, SIGNAL(clicked(bool)), this, SLOT(lataa()));

    // Ladataan kielet
    QStringList kielet = kp()->asetukset()->avaimet("tppohja/");
    for(QString kieli : kielet) {
        QString koodi = kieli.mid(8);
        QString kielitxt = kp()->asetukset()->kieli(koodi);
        ui->kieliCombo->addItem(QIcon(":/liput/" + koodi + ".png"), kielitxt, koodi);
    }
    // Sitten pitäisi valita nykyinen kieli
    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( kp()->asetus("tpkieli") ) );

    tarkistaPMA();
    connect( ui->kieliCombo, &QComboBox::currentTextChanged, this, &TpAloitus::lataa);
}

TpAloitus::~TpAloitus()
{
    delete ui;
}

void TpAloitus::valintaMuuttui(QStandardItem *item)
{
    // Valinta muuttuu, jolloin varmistetaan, ettei uuden valitun kanssa
    // poissulkevia ole valittu
    if( item->checkState() == Qt::Unchecked )
        return;

    QStringList poislista = item->data(PoisRooli).toStringList();
    foreach (QString poistettava, poislista) {
        for( int i=0; i < model->rowCount(QModelIndex()); i++)
        {
            if( model->item(i)->data(TunnusRooli) == poistettava)
            {
                if( model->item(i)->checkState() != Qt::Unchecked)
                    model->item(i)->setCheckState(Qt::Unchecked);
                continue;
            }
        }
    }
}

void TpAloitus::accept()
{
    talleta();
    QDialog::accept();
}

void TpAloitus::lataaTiedosto()
{
    QString tiedosto = QFileDialog::getOpenFileName(this, tr("Valitse tilinpäätöstiedosto"),
                                                    QDir::homePath(), tr("Pdf-tiedostot (*.pdf)"));
    if( !tiedosto.isEmpty() )
    {
        QFile luku(tiedosto);
        if(!luku.open(QIODevice::ReadOnly))
            QMessageBox::critical(this, tr("Tiedostovirhe"), tr("Tiedoston %1 luku epäonnistui\n%2")
                                  .arg(tiedosto)
                                  .arg(luku.errorString()));
        QMap<QString,QString> meta;
        meta.insert("Filename",tiedosto);
        KpKysely* kysely = kpk(QString("/liitteet/0/TP_%1").arg(tilikausi.paattyy().toString(Qt::ISODate)), KpKysely::PUT);
        connect( kysely, &KpKysely::vastaus, this, &TpAloitus::reject);
        kysely->lahetaTiedosto( luku.readAll(), meta);
    }
}

void TpAloitus::ohje()
{
    kp()->ohje("tilinpaatos/asiakirja");
}

void TpAloitus::tarkistaPMA()
{
    Tilikausi edellinen = kp()->tilikaudet()->tilikausiPaivalle( tilikausi.alkaa().addDays(-1) );


    if( kp()->asetukset()->asetus("muoto") == "tmi" &&
            tilikausi.pieniElinkeinonharjoittaja() < 2 &&
            edellinen.pieniElinkeinonharjoittaja() < 2)
    {
        // Elinkeinonharjoittajalla vähän toisenlainen        
        ui->saantoGroup->setVisible(false);
        ui->vapaaehtoisLabel->setVisible( true );
        ui->mikroRadio->setChecked(true);

    }
    else
    {
        ui->vapaaehtoisLabel->setVisible(false);


        int pienuus = tilikausi.pienuus();
        if( edellinen.pienuus() > pienuus)
            pienuus = edellinen.pienuus();

        ui->mikroRadio->setEnabled( pienuus == Tilikausi::MIKROYRITYS );
        ui->pienRadio->setEnabled( pienuus == Tilikausi::PIENYRITYS || pienuus == Tilikausi::MIKROYRITYS);

        ui->mikroRadio->setChecked( pienuus == Tilikausi::MIKROYRITYS);
        ui->pienRadio->setChecked( pienuus == Tilikausi::PIENYRITYS);
        ui->taysRadio->setChecked( pienuus == Tilikausi::YRITYS);
    }

    lataa();    // Lataa valinnat uudelleen
}

void TpAloitus::lataa()
{
    if(model)
        delete model;

    model = new QStandardItemModel;
    QStringList kaava = kp()->asetukset()->lista("tppohja/" + ui->kieliCombo->currentData().toString());

    QRegularExpression valintaRe("#(?<tunnus>\\w+)(?<pois>(\\s-\\w+)*)\\s(?<naytto>.+)");
    valintaRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression poisRe("-(?<pois>\\w+)");
    poisRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression ehtoRe("^\\?(?<avain>\\w+)=(?<arvo>.*)");
    ehtoRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);


    QFont lihava;
    lihava.setBold(true);
    bool ehtotulosta = true;    // Onko ?-ehto voimassa

    foreach (QString rivi, kaava)
    {
        // ?Avain=Arvo tulostumisen ehto asetuksissa
        if( rivi.startsWith('?'))
        {
            QRegularExpressionMatch emats = ehtoRe.match(rivi);
            if( emats.hasMatch())
            {
                QString avain = emats.captured("avain");
                QString ehtolause = emats.captured("arvo");

                ehtotulosta =  kp()->asetukset()->asetus(avain).contains(QRegularExpression(ehtolause));                
            }
            else
                // Jos ?-alkava rivi ei kelpo, niin tulostaa joka tapauksessa
                ehtotulosta = true;

            continue;
        }
        if( !ehtotulosta)
            continue;

        // Otsikkorivi
        if( rivi.startsWith("##"))
        {
            QStandardItem *item = new QStandardItem( rivi.mid(2));
            item->setFont( lihava );
            item->setFlags( Qt::ItemIsEnabled);
            model->appendRow(item);
        }
        else
        {
            // Onko valintarivi
            QRegularExpressionMatch mats = valintaRe.match(rivi);
            if( mats.hasMatch() )
            {
                QStandardItem *item = new QStandardItem( mats.captured("naytto") );
                item->setCheckable(true);
                item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
                item->setData( mats.captured("tunnus"), TunnusRooli );

                QString pois = mats.captured("pois");
                if( !pois.isEmpty())
                {
                    QStringList poisTunnukset;
                    QRegularExpressionMatchIterator iter = poisRe.globalMatch(pois);
                    while( iter.hasNext())
                        poisTunnukset.append( iter.next().captured(1) );

                    // Jos tämä ehto koskee toisen kokoluokan yritystä,
                    // ei valintaa tallenneta eikä siten myöskään näytetä

                    if(( poisTunnukset.contains("M") && ui->mikroRadio->isChecked()) ||
                       ( poisTunnukset.contains("P") && ui->pienRadio->isChecked()) ||
                       ( poisTunnukset.contains("I") && ui->taysRadio->isChecked()) )
                        continue;

                    item->setData( poisTunnukset, PoisRooli);
                }
                model->appendRow(item);
            }
        }

    }
    ui->view->setModel( model );

    // Ladataan viimeiset tallennetut valinnat
    QStringList valitut = kp()->asetukset()->asetus("tilinpaatosvalinnat").split(",");
    for(int i=0; i < model->rowCount(QModelIndex());i++)
    {
        if( model->item(i)->isCheckable() && valitut.contains(  model->item(i)->data(TunnusRooli).toString() ))
            model->item(i)->setCheckState(Qt::Checked);
    }

}

void TpAloitus::talleta()
{
    tilikausi.set("henkilosto", ui->henkilostoSpin->value());
    tilikausi.tallenna();

    kp()->asetukset()->aseta("tpkieli", ui->kieliCombo->currentData().toString());

    QStringList valitut;
    for(int i=0; i < model->rowCount(QModelIndex()); i++)
    {
        QStandardItem *item = model->item(i);
        if( item->checkState() == Qt::Checked)
            valitut.append( item->data(TunnusRooli).toString());
    }
    if( ui->mikroRadio->isChecked())
        valitut.append("MIKRO");
    else if( ui->pienRadio->isChecked())
        valitut.append("PIEN");
    else if( ui->taysRadio->isChecked())
        valitut.append("ISO");

    if( ui->henkilostoSpin->value() )       
        valitut.append("HENKILOSTO");


    kp()->asetukset()->aseta("tilinpaatosvalinnat",valitut.join(","));
}
