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

#include <QSqlQuery>

#include <QDebug>
#include <QSqlError>

#include "db/yhteysmodel.h"
#include "sqlite/sqlitemodel.h"
#include "sqlite/routes/tositeroute.h"

#include "tositeselausmodel.h"
#include "db/kirjanpito.h"

#include "db/kpkysely.h"
#include "db/tositetyyppimodel.h"

#include "model/tosite.h"

#include <algorithm>
#include <QJsonDocument>

TositeSelausModel::TositeSelausModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int TositeSelausModel::rowCount(const QModelIndex & /* parent */) const
{
    return rivit_.count();
}

int TositeSelausModel::columnCount(const QModelIndex & /* parent */) const
{
    return 6;
}

QVariant TositeSelausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole)
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section) {
        case TUNNISTE:
            if( tila_ < KIRJANPIDOSSA)
                return tr("Tila");
            else
                return tr("Tosite");
        case PVM:
            return tr("Pvm");
        case TOSITETYYPPI:
            return tr("Laji");
        case SUMMA:
            return tr("Summa");
        case ASIAKASTOIMITTAJA:
            return tr("Asiakas/Toimittaja");
        case OTSIKKO:
            return tr("Otsikko");
        }
    }
    return QVariant( section + 1);
}

QVariant TositeSelausModel::data(const QModelIndex &index, int role) const
{

    if( !index.isValid())
        return QVariant();

    return rivit_.at(index.row()).data(index.column(), role, tila_);
}

QList<int> TositeSelausModel::tyyppiLista() const
{
    QList<int> lista = kaytetytTyypit_.values();
    std::sort( lista.begin(), lista.end() );
    return lista;
}

QStringList TositeSelausModel::sarjaLista() const
{
    QList<QString> lista = kaytetytSarjat_.values();
    std::sort( lista.begin(), lista.end());
    return lista;
}



void TositeSelausModel::lataa(const QDate &alkaa, const QDate &loppuu, int tila)
{
    tila_ = tila;
    samakausi_ = kp()->tilikausiPaivalle(alkaa).alkaa() == kp()->tilikausiPaivalle(loppuu).alkaa();

    if( kp()->yhteysModel())
    {
        SQLiteModel* sqlite = qobject_cast<SQLiteModel*>( kp()->yhteysModel() );
        if( sqlite ) {
            lataaSqlite(sqlite, alkaa, loppuu);
        } else if(!ladataan_ && kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_SELAUS | YhteysModel::RAPORTIT | YhteysModel::TILINPAATOS) ){
            KpKysely *kysely = kpk("/tositteet");
            kysely->lisaaAttribuutti("alkupvm", alkaa);
            kysely->lisaaAttribuutti("loppupvm", loppuu);
            if( tila == LUONNOKSET )
                kysely->lisaaAttribuutti("luonnos", QString());
            else if( tila == SAAPUNEET)
                kysely->lisaaAttribuutti("saapuneet", QString());
            else if( tila == POISTETUT)
                kysely->lisaaAttribuutti("poistetut", QString());

            connect( kysely, &KpKysely::vastaus, this, &TositeSelausModel::tietoSaapuu);
            connect(kysely, &KpKysely::virhe, this, &TositeSelausModel::latausVirhe);
            ladataan_ = true;
            kysely->kysy();
        }
    }
}

void TositeSelausModel::tietoSaapuu(QVariant *var)
{
    beginResetModel();
    kaytetytTyypit_.clear();
    kaytetytSarjat_.clear();
    QVariantList lista = var->toList();

    rivit_.clear();
    rivit_.reserve(lista.count());

    for( const auto& item : qAsConst( lista )) {
        QVariantMap map = item.toMap();
        TositeSelausRivi rivi(map, samakausi_);
        rivit_.append(rivi);
        kaytetytTyypit_.insert( rivi.getTyyppi() );
        if( !rivi.getSarja().isEmpty() )
            kaytetytSarjat_.insert( rivi.getSarja() );
    }

    endResetModel();
    ladataan_ = false;
}

void TositeSelausModel::latausVirhe()
{
    ladataan_ = false;
}

void TositeSelausModel::lataaSqlite(SQLiteModel *sqlite, const QDate &alkaa, const QDate &loppuu)
{
    QStringList ehdot;
    if( tila_ == LUONNOKSET )
        ehdot.append( QString("( tosite.tila >= %1 and tosite.tila < %2 )").arg(Tosite::LUONNOS).arg(Tosite::KIRJANPIDOSSA) );
    else if( tila_ == SAAPUNEET)
        ehdot.append( QString("( tosite.tila > %1 and tosite.tila < %2 )").arg(Tosite::MALLIPOHJA).arg(Tosite::LUONNOS) );
    else if( tila_ == POISTETUT)
        ehdot.append("tosite.tila = 0");
    else
        ehdot.append( QString("tosite.tila >= %1").arg(Tosite::KIRJANPIDOSSA));

    ehdot.append( QString("tosite.pvm >= '%1'").arg( alkaa.toString(Qt::ISODate) ));
    ehdot.append( QString("tosite.pvm <= '%1'").arg( loppuu.toString(Qt::ISODate)));

    QString kysymys = "SELECT tosite.id AS id, tosite.pvm AS pvm, tyyppi, tila, tunniste, otsikko, kumppani.nimi as kumppani, "
                      "tosite.sarja as sarja, liitteita, summa, Tosite.json AS json"
                      " FROM Tosite LEFT OUTER JOIN Kumppani on tosite.kumppani=kumppani.id  "
                      " LEFT OUTER JOIN (SELECT tosite, COUNT(id) AS liitteita FROM Liite GROUP BY tosite) AS lq ON tosite.id=lq.tosite "
                      " LEFT OUTER JOIN (SELECT tosite, SUM(debetsnt) AS summa FROM Vienti GROUP BY tosite) as sq ON tosite.id=sq.tosite "
                      "WHERE ";
    kysymys.append( ehdot.join(" AND "));

    beginResetModel();
    kaytetytTyypit_.clear();
    kaytetytSarjat_.clear();

    rivit_.clear();

    QSqlQuery kysely(sqlite->tietokanta());
    kysely.exec(kysymys);
    while(kysely.next()) {
        TositeSelausRivi rivi(kysely, samakausi_);
        rivit_.append(rivi);
        kaytetytTyypit_.insert( rivi.getTyyppi() );
        if( !rivi.getSarja().isEmpty() )
            kaytetytSarjat_.insert( rivi.getSarja() );
    }

    endResetModel();
}

TositeSelausRivi::TositeSelausRivi(const QVariantMap &data, bool samakausi)
{
    tositeId = data.value("id").toInt();
    tila = data.value("tila").toInt();
    pvm = data.value("pvm").toDate();
    tositeTyyppi = data.value("tyyppi").toInt();
    sarja = data.value("sarja").toString();
    tositeTunniste = kp()->tositeTunnus(data.value("tunniste").toInt(),
                                        data.value("pvm").toDate(),
                                        sarja,
                                        samakausi);
    vertailuTunniste = kp()->tositeTunnus(data.value("tunniste").toInt(),
                                          data.value("pvm").toDate(),
                                          data.value("sarja").toString(),
                                          samakausi,
                                          true);
    otsikko = data.value("otsikko").toString();
    kumppani = data.value("kumppani").toString();
    summa = data.value("summa").toString();
    liitteita = data.value("liitteita").toInt();
    huomio = data.value("huomio").toBool();
    tilioimatta = data.value("tilioimatta").toInt();
    etsiTeksti = tositeTunniste + " " + kumppani + " " + otsikko;
}

TositeSelausRivi::TositeSelausRivi(QSqlQuery &data, bool samakausi)
{
    tositeId = data.value("id").toInt();
    tila = data.value("tila").toInt();
    pvm = data.value("pvm").toDate();
    tositeTyyppi = data.value("tyyppi").toInt();
    sarja = data.value("sarja").toString();
    tositeTunniste = kp()->tositeTunnus(data.value("tunniste").toInt(),
                                        data.value("pvm").toDate(),
                                        sarja,
                                        samakausi);
    vertailuTunniste = kp()->tositeTunnus(data.value("tunniste").toInt(),
                                          data.value("pvm").toDate(),
                                          data.value("sarja").toString(),
                                          samakausi,
                                          true);
    otsikko = data.value("otsikko").toString();
    kumppani = data.value("kumppani").toString();
    summa = data.value("summa").toLongLong();
    liitteita = data.value("liitteita").toInt();
    QJsonDocument json = QJsonDocument::fromJson(data.value("json").toByteArray());
    huomio = json["huomio"].toBool();
    etsiTeksti = tositeTunniste + " " + kumppani + " " + otsikko;

}

QVariant TositeSelausRivi::data(int sarake, int role, int selaustila) const
{
    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (sarake)
        {

        case TositeSelausModel::TUNNISTE:
            if( selaustila < TositeSelausModel::KIRJANPIDOSSA) {   // Tila
                if( tila == Tosite::LAHETETAAN)
                    return TositeSelausModel::tr("Lähettäminen epäonnistui");
                else
                    return Tosite::tilateksti( tila );   // TODO Tilojen nimet
            }
            return role == Qt::EditRole ? vertailuTunniste : tositeTunniste;
        case TositeSelausModel::PVM:
            if( role == Qt::DisplayRole)
                return pvm;
            else
                return QString("%1 %2").arg( pvm.toString(Qt::ISODate) ).arg( tositeId, 8, 10, QChar('0') );

        case TositeSelausModel::TOSITETYYPPI:
            return kp()->tositeTyypit()->nimi( tositeTyyppi ) ;   // TODO: Tyyppikoodien käsittely

        case TositeSelausModel::ASIAKASTOIMITTAJA:
            return kumppani;

        case TositeSelausModel::OTSIKKO:
            return otsikko;

        case TositeSelausModel::SUMMA:
            if( role == Qt::EditRole)
                return summa.cents();
            else
                return summa.display(false);
        }

    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( sarake == TositeSelausModel::SUMMA )
            return QVariant( Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter );
    }
    else if( role == Qt::UserRole )
    {
        // UserRolessa on id
        return tositeId;
    } else if( role == TositeSelausModel::EtsiRooli) {
        return etsiTeksti;
    }
    else if( role == Qt::DecorationRole && sarake==TositeSelausModel::SUMMA )
    {
        if(  liitteita && !huomio)
            return QIcon(":/pic/liite.png");
        else if( huomio && !liitteita)
            return QIcon(":/pic/huomio.png");
        else if(liitteita && huomio)
            return QIcon(":/pic/huomioliite.png");
        else
            return QIcon(":/pic/tyhja.png");
    }
    else if( role == Qt::DecorationRole && sarake == TositeSelausModel::TOSITETYYPPI )
        return kp()->tositeTyypit()->kuvake( tositeTyyppi );
    else if( role == Qt::DecorationRole && sarake == TositeSelausModel::TUNNISTE ) {
        if( tilioimatta )
            return QImage(":/pic/oranssi.png");
        return Tosite::tilakuva(tila);
    } else if( role == TositeSelausModel::TositeTyyppiRooli)
    {
        return tositeTyyppi;
    } else if( role == TositeSelausModel::TositeSarjaRooli) {
        return sarja;
    } else if( role == Qt::ForegroundRole && tilioimatta ) {
        return QVariant(QColor(Qt::red));
    }
    return QVariant();
}
