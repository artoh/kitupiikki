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
#include "huoneistomodel.h"
#include "../viitenumero.h"

#include "db/kirjanpito.h"
#include "rekisteri/asiakastoimittajalistamodel.h"

HuoneistoModel::HuoneistoModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &HuoneistoModel::paivita);
}

QVariant HuoneistoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case NIMI: return tr("Nimi");
        case ASIAKAS: return tr("Asiakas");
        case LASKUT: return tr("Laskutettu");
        case MAKSUT: return tr("Maksettu");
        case AVOIN: return tr("Avoinna");
        }
    }
    return QVariant();
}

int HuoneistoModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return huoneistot_.count();
}

int HuoneistoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 5;
    // FIXME: Implement me!
}

QVariant HuoneistoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const HuoneistoTieto& tieto = huoneistot_.at(index.row());


    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case NIMI: return tieto.nimi();
        case ASIAKAS:
            return AsiakasToimittajaListaModel::instanssi()->nimi( tieto.asiakas() );
        case LASKUT:
            return tieto.laskutettu().display(false);
        case MAKSUT:
            return tieto.maksettu().display(false);
        case AVOIN: {
            Euro avoin = tieto.laskutettu() - tieto.maksettu();
            return avoin.display(false);
        }
    }
    } else if( role == ViiteRooli) {
        ViiteNumero viite(ViiteNumero::HUONEISTO, tieto.id());
        return viite.viite();
    } else if( role == IdRooli ) {
        return tieto.id();
    } else if( role == Qt::TextAlignmentRole && index.column() >= LASKUT) {
        return Qt::AlignRight;
    } else if( role == NimiRooli || role == TekstiRooli) {
        return tieto.nimi();
    }

    // FIXME: Implement me!
    return QVariant();
}

void HuoneistoModel::paivita()
{
    KpKysely* kysely = kpk("/huoneistot");
    kysely->lisaaAttribuutti("saldopvm", kp()->paivamaara());
    connect( kysely, &KpKysely::vastaus, this, &HuoneistoModel::lataa);
    kysely->kysy();
}

void HuoneistoModel::lataa(QVariant *data)
{
    beginResetModel();
    huoneistot_.clear();
    QVariantList lista = data->toList();
    for(const auto& item : lista) {
        QVariantMap map = item.toMap();
        huoneistot_.append( HuoneistoTieto( map.value("id").toInt(),
                                            map.value("nimi").toString(),
                                            map.value("asiakas").toInt(),
                                            map.value("laskutettu").toString(),
                                            map.value("maksettu").toString()));
    }
    endResetModel();
}


HuoneistoModel::HuoneistoTieto::HuoneistoTieto()
{

}

HuoneistoModel::HuoneistoTieto::HuoneistoTieto(int id, const QString &nimi, int asiakas, const Euro &laskutettu, const Euro &maksettu)
    : id_(id), nimi_(nimi), asiakas_(asiakas), laskutettu_(laskutettu), maksettu_(maksettu)
{

}

int HuoneistoModel::HuoneistoTieto::id() const
{
    return id_;
}

QString HuoneistoModel::HuoneistoTieto::nimi() const
{
    return nimi_;
}

int HuoneistoModel::HuoneistoTieto::asiakas() const
{
    return asiakas_;
}

Euro HuoneistoModel::HuoneistoTieto::laskutettu() const
{
    return laskutettu_;
}

Euro HuoneistoModel::HuoneistoTieto::maksettu() const
{
    return maksettu_;
}

ViiteNumero HuoneistoModel::HuoneistoTieto::viite() const
{
    ViiteNumero viite(ViiteNumero::HUONEISTO, id());
    return viite;
}
