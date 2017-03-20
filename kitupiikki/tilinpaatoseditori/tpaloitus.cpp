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

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include "tpaloitus.h"
#include "ui_tpaloitus.h"

#include "db/kirjanpito.h"

#include <QDebug>

TpAloitus::TpAloitus(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TpAloitus)
{
    ui->setupUi(this);

    model = new QStandardItemModel;

    QStringList kaava = kp()->asetukset()->lista("LiitetietoKaava");

    QRegularExpression valintaRe("#(?<tunnus>\\w+)(?<pois>(\\s-\\w+)*)\\s(?<naytto>.+)");
    valintaRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression poisRe("-(?<pois>\\w+)");
    poisRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);

    foreach (QString rivi, kaava)
    {
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
    ui->view->setModel( model );

    connect( model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(valintaMuuttui(QStandardItem*)));
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
