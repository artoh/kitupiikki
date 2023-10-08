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
#include "laskutus/ryhmalasku/toimitustapadelegaatti.h"
#include "db/yhteysmodel.h"

#include "model/tosite.h"
#include "model/lasku.h"
#include "db/tositetyyppimodel.h"

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
                if( !map.value("numero").toString().isEmpty()) {
                    if( role == Qt::EditRole)
                        return QString(" %1").arg(map.value("numero").toString(), 20, '0');
                    return map.value("numero").toDouble() > 1e-5 ? QString::number(map.value("numero").toDouble(),'f',0) : map.value("numero");
                } else if(!map.value("viite").toString().isEmpty())
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
                    return Euro::fromVariant(map.value("summa")).display(false);
                }
                else
                   return map.value("summa");
            case MAKSAMATTA:
                if( role == Qt::DisplayRole)
                {
                   Euro avoin = Euro::fromVariant(map.value("avoin"));
                   return avoin.display(false);
                }
                else {
                   return map.value("avoin");
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
    case Qt::ForegroundRole:
            if( index.column() == ERAPVM )
                if( kp()->paivamaara().daysTo( map.value("erapvm").toDate() ) < 0 && map.value("avoin").toDouble() > 1e-5 ) {
                    const int tila = map.value("tila").toInt();
                    if( (tila == Tosite::LAHETETAAN || tila == Tosite::LAHETYSVIRHE) && QPalette().base().color().lightness() < 128 ) {
                        return QPalette().brightText().color();
                    }
                    return QBrush(QColor(Qt::red));
                }
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
            era.insert("tositetyyppi", map.value("tyyppi"));
            return era;
        }
    case NumeroRooli:
        return map.value("numero");
    case Qt::DecorationRole: {
        if( index.column() == NUMERO) {
                int tila = map.value("tila").toInt();
                if( tila == Tosite::LAHETETAAN)
                    return QIcon(":/pic/email.png");
                else if(tila == Tosite::LAHETYSVIRHE)
                    return QIcon(":/pic/mailfail.png");
                else if(tila == Tosite::TOIMITETTULASKU)
                    return QIcon(":/pic/mailok.png");
                else if( tila == Tosite::AVATTULASKU)
                    return QIcon(":/pic/mail-readed.png");

                switch (map.value("tyyppi").toInt()) {
                case TositeTyyppi::MYYNTILASKU:                    
                    if( map.value("maksutapa").toInt() == Lasku::KATEINEN )
                        return QIcon(":/pic/kateinen.png");
                    else if( map.value("maksutapa").toInt() == Lasku::KORTTIMAKSU )
                        return QIcon(":/pic/luottokortti.png");
                    else if( map.value("maksutapa").toInt() == Lasku::ENNAKKOLASKU)
                        return QIcon(":/pic/ennakkolasku.png");
                    else if( map.value("maksutapa").toInt() == Lasku::SUORITEPERUSTE)
                        return QIcon(":/pic/suorite.png");
                    else if( map.value("maksutapa").toInt() == Lasku::KUUKAUSITTAINEN)
                        return QIcon(":/pic/kuu.svg");
                    else if( map.value("valvonta").toInt() == Lasku::ASIAKAS)
                        return QIcon(":/pic/mies.png");
                    else if( map.value("valvonta").toInt() == Lasku::VAKIOVIITE)
                        return QIcon(":/pic/viivakoodi.png");
                    else if( map.value("valvonta").toInt() == Lasku::HUONEISTO)
                        return QIcon(":/pic/talo.png");
                    else if(map.value("valvonta").toInt() == Lasku::VALVOMATON)
                        return QIcon(":/pic/eikaytossa.png");
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
            } else if( index.column() == PVM) {
                if( map.value("toistopvm").isNull())
                    return QIcon(":/pic/tyhja.png");
                else
                    return QIcon(":/pic/refresh.png");
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
        return ostoja_ ? Tosite::KIRJANPIDOSSA : map.value("tila");
    case SummaRooli:
        return map.value("summa");
    case OtsikkoRooli:
        return map.value("otsikko");
    case OstoLaskutTieto:
        return ostoja_;
    case MapRooli:
        return map;
    case Qt::BackgroundRole:
    {
        const int tila = map.value("tila").toInt();
        if( tila == Tosite::LAHETETAAN || tila == Tosite::LAHETYSVIRHE ) {
            const bool alternateColor = index.row() % 2 == 1;
            if( QPalette().base().color().lightness() > 128) {
                return alternateColor ? QBrush(QColor(255, 200, 77)) : QBrush(QColor(255,209,102));
            } else {
                return alternateColor ? QBrush(QColor(204, 41, 0)) : QBrush(QColor(255,51,0));
            }

        }
        return QVariant();
    }
    default:
        return QVariant();
    }
}

void LaskuTauluModel::lataaAvoimetMaksettavat(bool ostoja)
{
    paivita(ostoja, MAKSETTAVAT );
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
    if( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_SELAUS | YhteysModel::TOSITE_LUONNOS | YhteysModel::TOSITE_MUOKKAUS))
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
    else if(valinta_ == MAKSETTAVAT)
        kysely->lisaaAttribuutti("avoin","maksut");
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
