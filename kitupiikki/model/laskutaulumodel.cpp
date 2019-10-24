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
#include "laskutaulumodel.h"

#include "db/kirjanpito.h"
#include "laskutus/laskudialogi.h"

LaskuTauluModel::LaskuTauluModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant LaskuTauluModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case NUMERO: return tr("Viitenumero");
        case PVM: return tr("Laskun pvm");
        case ERAPVM: return tr("Eräpvm");
        case SUMMA: return tr("Summa");
        case LAHETYSTAPA: return tr("Lähetystapa");
        case MAKSAMATTA: return tr("Maksamatta");
        case ASIAKASTOIMITTAJA:
            if( ostoja_)
                return tr("Toimittaja/Selite");
            return tr("Asiakas/Selite");
        }
    }
    return QVariant();
}

int LaskuTauluModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return lista_.count();
}

int LaskuTauluModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 7;
}

QVariant LaskuTauluModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = lista_.at(index.row()).toMap();

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            switch (index.column())
            {
            case NUMERO:
                return map.value("viite");
            case PVM:
                return map.value("pvm").toDate();
            case ERAPVM:
    //            if( lasku.kirjausperuste == LaskuModel::KATEISLASKU ||  lasku.json.luku("Hyvityslasku"))
    //                return QString();
                return map.value("erapvm").toDate();
            case SUMMA:
                if( role == Qt::DisplayRole)
                {
                    double summa = map.value("summa").toDouble();
                    if( summa > 1e-5)
                        return QString("%L1 €").arg(summa,0,'f',2);
                    else
                        return QVariant();  // Nollalle tyhjää
                }
                else
                   return map.value("summa").toDouble();
            case MAKSAMATTA:
                if( role == Qt::DisplayRole)
                {                                     
                    double avoin = map.value("avoin").toDouble();
                    if( avoin > 1e-5)
                        return QString("%L1 €").arg( avoin ,0,'f',2);
                    else
                        return QVariant();  // Nollalle tyhjää
                }
                else
                    return map.value("avoin").toDouble();
            case LAHETYSTAPA:
            {
                switch (map.value("laskutapa").toInt()) {
                case LaskuDialogi::TULOSTETTAVA:
                    return tr("Tuloste");
                case LaskuDialogi::SAHKOPOSTI:
                    return tr("Sähköposti");
                default:
                    return QVariant();
                }
            }
            case ASIAKASTOIMITTAJA: {
                QString kumppani = ostoja_ ?
                            map.value("toimittaja").toString() : map.value("asiakas").toString();
                return kumppani.isEmpty() ? map.value("selite") : kumppani;
            }
            case OTSIKKO:
                return map.value("otsikko").toString();
            default:
                return QVariant();
            }
        }
    case Qt::TextAlignmentRole:
            if( index.column() == SUMMA || index.column() == MAKSAMATTA )
                return QVariant( Qt::AlignRight | Qt::AlignVCenter);
        return QVariant();
    case Qt::TextColorRole:
            if( index.column() == ERAPVM )
                if( kp()->paivamaara().daysTo( map.value("erapvm").toDate() ) < 0 && map.value("avoin") > 0 )
                    return QColor(Qt::red);
            return QVariant();
    case AvoinnaRooli:
        return map.value("avoin");
    case EraIdRooli:
        return map.value("eraid");
    case LaskuPvmRooli:
        return map.value("pvm");
    case AsiakasToimittajaNimiRooli:
        return ostoja_ ? map.value("toimittaja") : map.value("asiakas");
    case TiliRooli:
        return map.value("tili");
    case ViiteRooli :
        return map.value("viite");
    case AsiakasToimittajaIdRooli:
        return ostoja_ ? map.value("kumppani") : map.value("asiakasid");
    case TositeIdRooli:
        return map.value("tosite");
    case TyyppiRooli:
        return map.value("tyyppi");
    }
    return QVariant();
}

void LaskuTauluModel::lataaAvoimet(bool ostoja)
{
    paivita(ostoja, AVOIMET );
}

void LaskuTauluModel::paivita(bool ostoja, int valinta, QDate mista, QDate mihin)
{
    if( !kp()->yhteysModel())
        return;

    ostoja_ = ostoja;
    KpKysely *kysely = nullptr;

    if( ostoja )
        kysely = kpk("/ostolaskut");
    else
        kysely = kpk("/myyntilaskut");

    if( mista.isValid())
        kysely->lisaaAttribuutti("alkupvm", mista);

    if( mihin.isValid())
        kysely->lisaaAttribuutti("loppupvm", mihin);

    if( valinta == AVOIMET)
        kysely->lisaaAttribuutti("avoin",QString());
    else if( valinta == ERAANTYNEET)
        kysely->lisaaAttribuutti("eraantynyt",QString());
    else if( valinta == LUONNOS)
        kysely->lisaaAttribuutti("luonnos");
    else if( valinta == LAHETETTAVA)
        kysely->lisaaAttribuutti("lahetettava");

    connect( kysely, &KpKysely::vastaus, this, &LaskuTauluModel::tietoSaapuu);
    kysely->kysy();
}

void LaskuTauluModel::tietoSaapuu(QVariant *var)
{
    beginResetModel();
    lista_ = var->toList();
    endResetModel();
}
