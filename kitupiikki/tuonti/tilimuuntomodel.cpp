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

#include "tilimuuntomodel.h"
#include "db/tilinvalintadialogi.h"
#include "db/kirjanpito.h"

TilinMuunnos::TilinMuunnos(int numero, QString nimi, int muunnettu)
    : alkuperainenTilinumero(numero), tilinNimi(nimi)
{
    if( muunnettu)
        muunnettuTilinumero = muunnettu;
    else
        muunnettuTilinumero = numero;
}

QString TilinMuunnos::tiliStr() const
{
    if( alkuperainenTilinumero )
        return QString::number(alkuperainenTilinumero);
    else
        return tilinNimi;
}


TiliMuuntoModel::TiliMuuntoModel(const QList<QPair<int, QString>> &tilit)
{
    // Luetaan asetuksista aiemmat muunnot
    const QStringList& aiempiMuuntoLista = kp()->asetukset()->lista("TiliMuunto");

    for(auto rivi : aiempiMuuntoLista)
    {
        int vali = rivi.indexOf(' ');
        if( vali < 1)
            continue;
        int tili = rivi.left(vali).toInt();
        if( tili )
        {
            muunteluLista_.insert(rivi.mid(vali+1), tili);
        }
    }

    for( auto pari : tilit)
    {
       int muunnettu = muunteluLista_.value(pari.second, 0);
       data_.append(TilinMuunnos(pari.first, pari.second, muunnettu));
    }
}

int TiliMuuntoModel::rowCount(const QModelIndex & /* parent */) const
{
    return data_.count();
}

int TiliMuuntoModel::columnCount(const QModelIndex& /* parent */) const
{
    return 3;
}

QVariant TiliMuuntoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case ALKUPERAINEN:
            return tr("Numero");
        case NIMI:
            return tr("Nimi");
        case UUSI:
            return tr("Kirjataan tilille");
        }
    }

    return QVariant();

}

QVariant TiliMuuntoModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    TilinMuunnos rivi = data_.value(index.row());

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column()) {
        case ALKUPERAINEN:
           return rivi.alkuperainenTilinumero;
        case NIMI:
            return rivi.tilinNimi;
        case UUSI:
            if( role == Qt::EditRole)
                return rivi.muunnettuTilinumero;

            Tili tili= kp()->tilit()->tiliNumerolla( rivi.muunnettuTilinumero );
            if( tili.onkoValidi())
                return QString("%1 %2").arg(tili.numero()).arg(tili.nimi());
        }
    }
    return QVariant();
}

bool TiliMuuntoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( index.column() == UUSI && role == Qt::EditRole)
    {
        if( value.toInt())
            data_[index.row()].muunnettuTilinumero = value.toInt();
        else
        {
            Tili uusitili;
            if( value.toString() == " " || value.toString()=="0")
                uusitili = TilinValintaDialogi::valitseTili( QString());
            else
                uusitili = TilinValintaDialogi::valitseTili( value.toString());
            data_[index.row()].muunnettuTilinumero = uusitili.numero();
        }
        return true;
    }
    return false;
}

Qt::ItemFlags TiliMuuntoModel::flags(const QModelIndex &index) const
{
    if( index.column() == UUSI )
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index);
}

QMap<QString, int> TiliMuuntoModel::muunnettu()
{
    QMap<QString,int> tulos;
    for( auto rivi : data_)
    {        
        tulos.insert( rivi.tiliStr(), rivi.muunnettuTilinumero);
        muunteluLista_.insert( rivi.tiliStr(), rivi.muunnettuTilinumero );
    }

    // Lopuksi päivitetään muuntotaulukko
    QStringList listaan;
    auto iter = QMapIterator<QString,int>(muunteluLista_);
    while(iter.hasNext())
    {
        iter.next();
        listaan.append( QString("%1 %2").arg(iter.value()).arg(iter.key()) );
    }

    kp()->asetukset()->aseta("TiliMuunto", listaan);


    return tulos;
}

