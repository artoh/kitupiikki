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
#include "tpaloitus.h"
#include "ui_tpaloitus.h"

#include "db/kirjanpito.h"

#include <QDebug>
#include <QDesktopServices>

TpAloitus::TpAloitus(Tilikausi kausi, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TpAloitus),
    tilikausi(kausi)
{
    ui->setupUi(this);

    model = new QStandardItemModel;

    QStringList kaava = kp()->asetukset()->lista("TilinpaatosPohja");

    QRegularExpression valintaRe("#(?<tunnus>\\w+)(?<pois>(\\s-\\w+)*)\\s(?<naytto>.+)");
    valintaRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression poisRe("-(?<pois>\\w+)");
    poisRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QFont lihava;
    lihava.setBold(true);

    foreach (QString rivi, kaava)
    {
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
                    item->setData( poisTunnukset, PoisRooli);

                }
                model->appendRow(item);
            }
        }

    }
    ui->view->setModel( model );

    // Ladataan
    QStringList valitut = kp()->asetukset()->lista("TilinpaatosValinnat");
    for(int i=0; i < model->rowCount(QModelIndex());i++)
    {
        if( model->item(i)->isCheckable() && valitut.contains(  model->item(i)->data(TunnusRooli).toString() ))
            model->item(i)->setCheckState(Qt::Checked);
    }


    connect( model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(valintaMuuttui(QStandardItem*)));
    connect( ui->jatkaNappi, SIGNAL(clicked(bool)), this, SLOT(tallenna()));
    connect( ui->lataaNappi, SIGNAL(clicked(bool)), this, SLOT(lataa()));
    connect( ui->ohjeNappi, SIGNAL(clicked(bool)), this, SLOT(ohje()));
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

void TpAloitus::tallenna()
{
    QStringList valitut;
    for(int i=0; i < model->rowCount(QModelIndex()); i++)
    {
        QStandardItem *item = model->item(i);
        if( item->checkState() == Qt::Checked)
            valitut.append( item->data(TunnusRooli).toString());
    }
    kp()->asetukset()->aseta("TilinpaatosValinnat",valitut);
    accept();
}

void TpAloitus::lataa()
{
    QString tiedosto = QFileDialog::getOpenFileName(this, tr("Valitse vahvistettu tilinpäätöstiedosto"),
                                                    QDir::homePath(), tr("Pdf-tiedostot (*.pdf)"));
    if( !tiedosto.isEmpty() )
    {
        QString tulosfile =  kp()->hakemisto().absoluteFilePath("arkisto/" + tilikausi.arkistoHakemistoNimi() + "/tilinpaatos.pdf" );
        if( QFile::exists(tulosfile))
            QFile(tulosfile).remove();
        QFile::copy( tiedosto, tulosfile);

        kp()->tilikaudet()->vaihdaTilinpaatostila( kp()->tilikaudet()->indeksiPaivalle(tilikausi.paattyy()) ,  Tilikausi::VAHVISTETTU);

        reject();
    }
}

void TpAloitus::ohje()
{
    QDesktopServices::openUrl( QUrl("https://artoh.github.io/kitupiikki/tilinpaatos/"));
}
