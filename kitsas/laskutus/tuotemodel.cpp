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

#include "tuotemodel.h"
#include "db/kirjanpito.h"

#include "yksikkomodel.h"

#include "raportti/raportinkirjoittaja.h"

#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>

TuoteModel::TuoteModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int TuoteModel::rowCount(const QModelIndex & /* parent */) const
{
    return tuotteet_.count();
}

int TuoteModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant TuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if( section == NIMIKE )
            return tr("Nimike");
        else if( section == NETTO)
            return tr("Nettohinta");
        else if(section == BRUTTO)
            return tr("Bruttohinta");
    }        
    return QVariant();
}

QVariant TuoteModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    
    const Tuote &tuote = tuotteet_.at(index.row()).toMap();
    if( role == Qt::DisplayRole)
    {
        if( index.column() == NIMIKE )
            return tuote.nimike();
        else if( index.column() == NETTO)
        {            
            return QString("%L1 €").arg( tuote.ahinta() ,0,'f', desimaalit_);
        }
        else if(index.column() == BRUTTO)
        {
            double netto = tuote.ahinta();

            double alvprossa = tuote.alvkoodi() == AlvKoodi::MYYNNIT_NETTO ?
                    tuote.alvprosentti() : 0.0;

            double brutto = netto * (100 + alvprossa) / 100.0;

            return QString("%L1 €").arg( brutto ,0,'f',desimaalit_);
        }
    }
    else if( role == IdRooli)
        return tuote.id();
    else if( role == MapRooli)
        return tuote.toMap();

    return QVariant();
    
}

QString TuoteModel::nimike(int id) const
{
    for(const auto& rivi: tuotteet_) {
        if( rivi.id() == id) {
            return rivi.nimike();
        }
    }
    return QString();
}

QByteArray TuoteModel::csv() const
{
    RaportinKirjoittaja rk(true);            

    RaporttiRivi otsikko;
    otsikko.lisaa("id");
    otsikko.lisaa("koodi");
    otsikko.lisaa("nimike");
    otsikko.lisaa("yksikko");
    otsikko.lisaa("nettohinta");
    otsikko.lisaa("kohdennus");
    otsikko.lisaa("tili");
    otsikko.lisaa("alvkoodi");
    otsikko.lisaa("alvprosentti");
    rk.lisaaOtsake(otsikko);

    for(const auto& tuote : tuotteet_) {
        RaporttiRivi rivi;
        rivi.lisaa(QString::number(tuote.id()));
        rivi.lisaa(tuote.koodi());
        rivi.lisaa(tuote.nimike());
        rivi.lisaa(tuote.yksikko().isEmpty() ? tuote.unKoodi() : tuote.yksikko());
        rivi.lisaa(Euro::fromDouble(tuote.ahinta()).toString());
        rivi.lisaa(QString::number(tuote.kohdennus()));
        rivi.lisaa(QString::number(tuote.tili()));
        rivi.lisaa(QString::number(tuote.alvkoodi()));
        rivi.lisaa(QString::number(tuote.alvprosentti()));
        rk.lisaaRivi(rivi);
    }
    return rk.csv();
}

Tuote TuoteModel::tuote(int id) const
{
    for(const auto& item : tuotteet_) {
        if( item.id() == id)
            return item;
    }
    return Tuote();
}


void TuoteModel::lataa()
{
    KpKysely *kys = kpk("/tuotteet");
    if( kys ) {
        connect( kys, &KpKysely::vastaus, this, &TuoteModel::dataSaapuu);
        kys->kysy();
    }
}

void TuoteModel::paivitaTuote(Tuote tuote)
{    
    KpKysely *kysely = tuote.id() ?
                kpk(QString("/tuotteet/%1").arg(tuote.id()), KpKysely::PUT) :
                kpk("/tuotteet", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &TuoteModel::muokattu);
    kysely->kysy(tuote.toMap());
}

void TuoteModel::poistaTuote(int id)
{
    KpKysely *kysely = kpk(QString("/tuotteet/%1").arg(id), KpKysely::DELETE);
    kysely->kysy();

    for(int i=0; i<tuotteet_.count(); i++) {
        if( tuotteet_.at(i).id() == id) {
            beginRemoveRows(QModelIndex(),i,i);
            tuotteet_.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}


void TuoteModel::dataSaapuu(QVariant *data)
{
    beginResetModel();
    tuotteet_.clear();
    for(const auto& item : data->toList()) {
        tuotteet_.append(Tuote(item.toMap()));
    }

    desimaalit_ = kp()->asetukset()->luku("LaskuYksikkoDesimaalit", 2);

    endResetModel();
}

void TuoteModel::muokattu(QVariant *data)
{
    Tuote tuote( data->toMap());
    int id = tuote.id();
    for(int i=0; i<tuotteet_.count(); i++) {
        if( tuotteet_.at(i).id() == id) {
            tuotteet_[i] = tuote;
            emit dataChanged( index(i,0), index(i,BRUTTO) );
            return;
        }
    }
    // Ei löytynyt, lisätään
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    tuotteet_.append(tuote);
    endInsertRows();
}
