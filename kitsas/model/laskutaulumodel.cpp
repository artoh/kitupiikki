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
#include "laskutus/ryhmalasku/toimitustapadelegaatti.h"
#include "db/yhteysmodel.h"

LaskuTauluModel::LaskuTauluModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &LaskuTauluModel::paivitaNakyma);
}

QVariant LaskuTauluModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case NUMERO: return tr("Numero");
        case PVM: return tr("Laskun pvm");
        case ERAPVM: return tr("Eräpvm");
        case SUMMA: return tr("Summa");
        case LAHETYSTAPA: return tr("Lähetystapa");
        case MAKSAMATTA: return tr("Maksamatta");
        case ASIAKASTOIMITTAJA:
            if( ostoja_)
                return tr("Toimittaja");
            return tr("Asiakas");
        case OTSIKKO:
            return tr("Selite");
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

    return 8;
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
                if( map.contains("numero")) {
                    if( role == Qt::EditRole)
                        return QString(" %1").arg(map.value("numero").toLongLong(),20,10);
                    if(map.value("numero").toInt())
                        return map.value("numero").toLongLong();
                    return map.value("numero");
                } else if(map.contains("viite"))
                    return map.value("viite");
                else if(map.value("tunniste").toInt()){
                    return kp()->tositeTunnus(map.value("tunniste").toInt(),
                                              map.value("tositepvm").toDate(),
                                              map.value("sarja").toString(),
                                              false);
                } else {
                    return QVariant();
                }
            case PVM:
                return map.value("pvm").toDate();
            case ERAPVM:
                return map.value("erapvm").toDate();
            case SUMMA:
                if( role == Qt::DisplayRole)
                {
                    double summa = map.value("summa").toDouble();
                    if( qAbs(summa) > 1e-5)
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
                    if( qAbs(avoin) > 1e-5)
                        return QString("%L1 €").arg( avoin ,0,'f',2);
                    else
                        return QVariant();  // Nollalle tyhjää
                }
                else {
                    return map.value("avoin").toDouble();
                }
            case LAHETYSTAPA:
            {
                return ToimitustapaDelegaatti::toimitustapa(map.value("laskutapa").toInt());
            }
            case ASIAKASTOIMITTAJA: {
                return ostoja_ ?
                            map.value("toimittaja").toString() : map.value("asiakas").toString();                
            }
            case OTSIKKO:
            {
                QString kumppani = ostoja_ ? map.value("toimittaja").toString() : map.value("asiakas").toString();
                QString selite = map.value("selite").toString();
                if( kumppani == selite)
                    return QVariant();
                return map.value("selite").toString();
            }
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
                if( kp()->paivamaara().daysTo( map.value("erapvm").toDate() ) < 0 && map.value("avoin").toDouble() > 1e-5 )
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
        return ostoja_ ? map.value("toimittajaid") : map.value("asiakasid");
    case TositeIdRooli:
        return map.value("tosite");
    case TyyppiRooli:
        return map.value("tyyppi");
    case TunnisteRooli:
        return map.value("tunniste");
    case SarjaRooli:
        return map.value("sarja");
    case EraMapRooli: {
            QVariantMap era;
            era.insert("id", map.value("eraid"));
            era.insert("pvm", map.value("pvm"));
            era.insert("tunniste", map.value("tunniste"));
            era.insert("sarja", map.value("sarja"));
            era.insert("saldo", map.value("avoin"));
            return era;
        }
    case NumeroRooli:
        return map.value("numero");
    case Qt::DecorationRole: {
        if( index.column() == NUMERO) {
                int tila = map.value("tila").toInt();
                if( tila == Tosite::LAHETETAAN)
                    return QIcon(":/pic/varoitus.png");
                else if(tila == Tosite::LAHETYSVIRHE)
                    return QIcon(":/pic/mailfail.png");
                else if(tila == Tosite::TOIMITETTULASKU)
                    return QIcon(":/pic/mailok.png");

                switch (map.value("tyyppi").toInt()) {
                case TositeTyyppi::MYYNTILASKU:                    
                    if( map.value("maksutapa").toInt() == LaskuDialogi::KATEINEN )
                        return QIcon(":/pic/kateinen.png");
                    else if( map.value("maksutapa").toInt() == LaskuDialogi::ENNAKKOLASKU)
                        return QIcon(":/pic/ennakkolasku.png");
                    else if( map.value("maksutapa").toInt() == LaskuDialogi::SUORITEPERUSTE)
                        return QIcon(":/pic/suorite.png");
                    return QIcon(":/pic/lasku.png");
                case TositeTyyppi::HYVITYSLASKU:
                    return QIcon(":/pic/poista.png");
                case TositeTyyppi::MAKSUMUISTUTUS:
                    return QIcon(":/pic/punainenkuori.png");
                }
                return kp()->tositeTyypit()->kuvake(map.value("tyyppi").toInt());

            } else if( index.column() == LAHETYSTAPA) {
                return ToimitustapaDelegaatti::icon(map.value("laskutapa").toInt());
            } else if( index.column() == ERAPVM) {
            if( map.value("tila").toInt() == Tosite::MUISTUTETTU)
                return QIcon(":/pic/punainenkuori.png");
            else
                return QIcon(":/pic/tyhja.png");
            }
        }
        return QVariant();
    case EraPvmRooli:
        return map.value("erapvm").toDate();
    case LaskutustapaRooli:
        return map.value("laskutapa");
    case SeliteRooli:
        return map.value("selite");
    case TilaRooli:
        return map.value("tila");
    case SummaRooli:
        return map.value("summa").toDouble();
    case OtsikkoRooli:
        return map.value("otsikko");
    case OstoLaskutTieto:
        return ostoja_;
    }


    return QVariant();
}

void LaskuTauluModel::lataaAvoimet(bool ostoja)
{
    paivita(ostoja, AVOIMET );
}

void LaskuTauluModel::paivita(bool ostoja, int valinta, QDate mista, QDate mihin)
{
    ostoja_ = ostoja;
    valinta_ = valinta;
    mista_ = mista;
    mihin_ = mihin;
    paivitaNakyma();
}

void LaskuTauluModel::paivitaNakyma()
{
    if( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_SELAUS))
        return;


    KpKysely *kysely = nullptr;

    if( ostoja_ )
        kysely = kpk("/ostolaskut");
    else
        kysely = kpk("/myyntilaskut");

    if( mista_.isValid())
        kysely->lisaaAttribuutti("alkupvm", mista_);

    if( mihin_.isValid())
        kysely->lisaaAttribuutti("loppupvm", mihin_);

    if( valinta_ == AVOIMET)
        kysely->lisaaAttribuutti("avoin",QString());
    else if( valinta_ == ERAANTYNEET) {
        kysely->lisaaAttribuutti("eraantynyt");
        kysely->lisaaAttribuutti("eraloppupvm", kp()->paivamaara() );
    }
    else if( valinta_ == LUONNOS)
        kysely->lisaaAttribuutti("luonnos");
    else if( valinta_ == LAHETETTAVA)
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
