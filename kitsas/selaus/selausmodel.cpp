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

#include "selausmodel.h"

#include <QSqlQuery>
#include <QSqlError>
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"
#include "tositeselausmodel.h"

#include "sqlite/sqlitemodel.h"
#include <QDebug>


SelausModel::SelausModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int SelausModel::rowCount(const QModelIndex & /* parent */) const
{
    return rivit_.count();
}

int SelausModel::columnCount(const QModelIndex & /* parent */) const
{
    return 8;
}

QVariant SelausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole)
        return QVariant();
    else if( orientation == Qt::Horizontal )
    {
        switch (section)
        {
        case TOSITE:
            return QVariant("Tosite");
        case PVM :
            return QVariant("Pvm");
        case TILI:
            return QVariant("Tili");
        case DEBET :
            return QVariant("Debet");
        case KREDIT:
            return QVariant("Kredit");
        case KOHDENNUS:
            return QVariant("Kohdennus");
        case KUMPPANI:
            return QVariant("Asiakas/Toimittaja");
        case SELITE:
            return QVariant("Selite");
        }
    }

    return QVariant( section + 1  );
}

QVariant SelausModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    return rivit_.at(index.row()).data(index.column(), role);
}

void SelausModel::lataaSqlite(SQLiteModel* sqlite, const QDate &alkaa, const QDate &loppuu)
{

    QString kysymys = QString("SELECT vienti.id AS id, vienti.pvm as pvm, vienti.tili as tili, debetsnt, kreditsnt, "
                    "selite, vienti.kohdennus as kohdennus, eraid, vienti.tosite as tosite_id, tosite.pvm as tosite_pvm, tosite.tunniste as tosite_tunniste,"
                    "tosite.tyyppi as tosite_tyyppi, tosite.sarja as tosite_sarja, "
                    "kumppani.nimi as kumppani_nimi "
                    "FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                    "LEFT OUTER JOIN Kumppani ON Vienti.kumppani=kumppani.id "
                    "WHERE tila >= 100 AND Vienti.pvm BETWEEN '%1' AND '%2' ")
            .arg(alkaa.toString(Qt::ISODate))
            .arg(loppuu.toString(Qt::ISODate));

    beginResetModel();

    kaytetytTilit_.clear();
    rivit_.clear();

    QSqlQuery kysely( sqlite->tietokanta() );
    kysely.exec(kysymys);

    while(kysely.next()) {
        SelausRivi rivi(kysely, samakausi_, sqlite);
        rivit_.append(rivi);
        kaytetytTilit_.insert(rivi.getTili());
    }

    qDebug() << kysymys;
    qDebug() << kysely.lastError().text();

    endResetModel();

}

QList<int> SelausModel::tiliLista() const
{
    QList<int> lista = kaytetytTilit_.toList();
    std::sort(lista.begin(), lista.end());
    return lista;
}


void SelausModel::lataa(const QDate &alkaa, const QDate &loppuu)
{
    samakausi_ = kp()->tilikausiPaivalle(alkaa).alkaa() == kp()->tilikausiPaivalle(loppuu).alkaa();

    if( kp()->yhteysModel()) {

        SQLiteModel* sqlite = qobject_cast<SQLiteModel*>( kp()->yhteysModel() );
        if( sqlite ) {
            lataaSqlite(sqlite, alkaa, loppuu);
        } else {
            KpKysely *kysely = kpk("/viennit");
            if(kysely) {
                kysely->lisaaAttribuutti("alkupvm", alkaa);
                kysely->lisaaAttribuutti("loppupvm", loppuu);
                connect( kysely, &KpKysely::vastaus, this, &SelausModel::tietoSaapuu);

                kysely->kysy();
            }
        }
    }
    return;
}

void SelausModel::tietoSaapuu(QVariant *var)
{

    beginResetModel();
    kaytetytTilit_.clear();

    QVariantList lista = var->toList();
    rivit_.clear();


    for(auto item : lista)
    {
        QVariantMap map = item.toMap();
        SelausRivi rivi(map, samakausi_);
        rivit_.append(rivi);
        kaytetytTilit_.insert(rivi.getTili());
    }

    endResetModel();
}

SelausRivi::SelausRivi(const QVariantMap &data, bool samakausi)
{
    QVariantMap tosite = data.value("tosite").toMap();

    vientiId = data.value("id").toInt();
    tositeId = tosite.value("id").toInt();
    tositeTunnus = kp()->tositeTunnus(tosite.value("tunniste").toInt(),
                                      tosite.value("pvm").toDate(),
                                      tosite.value("sarja").toString(),
                                      samakausi);
    vertailuTunnus = kp()->tositeTunnus(tosite.value("tunniste").toInt(),
                                        tosite.value("pvm").toDate(),
                                        tosite.value("sarja").toString(),
                                        samakausi,
                                        true);
    pvm = data.value("pvm").toDate();
    tositeTyyppi = tosite.value("tyyppi").toInt();
    tili = data.value("tili").toInt();
    debet = data.value("debet").toDouble();
    kredit = data.value("kredit").toDouble();
    kumppani = data.value("kumppani").toMap().value("nimi").toString();
    selite = data.value("selite").toString();

    etsi = tositeTunnus + " " + kumppani + " " + selite;
    maksettu = false;

    Kohdennus kohdennusObj =  kp()->kohdennukset()->kohdennus( data.value("kohdennus").toInt() );
    kohdennustyyppi = kohdennusObj.tyyppi();
    if(kohdennustyyppi)
       kohdennus = kohdennusObj.nimi();

    if( data.contains("merkkaukset")) {
        QStringList tagit;
        for( auto merkkausVar : data.value("merkkaukset").toList()) {
            tagit.append( kp()->kohdennukset()->kohdennus(merkkausVar.toInt()).nimi() );
        }
        if( !kohdennus.isEmpty())
            kohdennus.append(" ");
        kohdennus.append( tagit.join(", "));
    }
    QVariantMap eraMap = data.value("era").toMap();
    if( eraMap.contains("tunniste")) {
        if(eraMap.value("id").toInt() != data.value("id").toInt()) {
            if(!kohdennus.isEmpty())
                kohdennus.append(" ");
            kohdennus.append( kp()->tositeTunnus(eraMap.value("tunniste").toInt(),
                                                 eraMap.value("pvm").toDate(),
                                                 eraMap.value("sarja").toString(),
                                                 kp()->tilikaudet()->tilikausiPaivalle(eraMap.value("pvm").toDate()).alkaa() == kp()->tilikaudet()->tilikausiPaivalle(pvm).alkaa() ));
        }
        maksettu = qAbs(eraMap.value("saldo").toDouble()) < 1e-5;
    }

}

SelausRivi::SelausRivi(QSqlQuery &data, bool samakausi, SQLiteModel *sqlite)
{
    vientiId = data.value("id").toInt();
    tositeId = data.value("tosite_id").toInt();
    tositeTunnus = kp()->tositeTunnus(data.value("tosite_tunniste").toInt(),
                                      data.value("tosite_pvm").toDate(),
                                      data.value("tosite_sarja").toString(),
                                      samakausi);
    vertailuTunnus = kp()->tositeTunnus(data.value("tosite_tunniste").toInt(),
                                        data.value("tosite_pvm").toDate(),
                                        data.value("tosite_sarja").toString(),
                                        samakausi,
                                        true);
    pvm = data.value("pvm").toDate();
    tositeTyyppi = data.value("tosite_tyyppi").toInt();
    tili = data.value("tili").toInt();
    debet = data.value("debetsnt").toDouble() / 100.0;
    kredit = data.value("kreditsnt").toDouble() / 100.0;
    kumppani = data.value("kumppani_nimi").toString();
    selite = data.value("selite").toString();

    etsi = tositeTunnus + " " + kumppani + " " + selite;
    maksettu = false;

    Kohdennus kohdennusObj = kp()->kohdennukset()->kohdennus(data.value("kohdennus").toInt());
    kohdennustyyppi = kohdennusObj.tyyppi();
    if(kohdennustyyppi)
       kohdennus = kohdennusObj.nimi();

    QSqlQuery apukysely( sqlite->tietokanta());
    apukysely.exec(QString("SELECT kohdennus FROM Merkkaus WHERE vienti=%1").arg(vientiId));
    QStringList tagit;
    while( apukysely.next()) {
        tagit.append( kp()->kohdennukset()->kohdennus(apukysely.value(0).toInt()).nimi() );
    }
    if( !tagit.isEmpty()) {
        if(!kohdennus.isEmpty())
            kohdennus.append(" ");
        kohdennus.append(tagit.join(", "));
    }

    int eraid = data.value("eraid").toInt();
    if( eraid) {

        // Tarkistetaan saldo, onko maksettu
        apukysely.exec(QString("SELECT SUM(debetsnt), SUM(kreditsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE eraid=%1 AND Tosite.tila >= 100").arg(eraid));
        if(apukysely.next() && apukysely.value(0).toLongLong() == apukysely.value(1).toLongLong())
            maksettu = true;

        if( eraid != vientiId) {
            apukysely.exec(QString("SELECT Tosite.tunniste, Tosite.sarja, Tosite.pvm AS pvm FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE Vienti.id=%1").arg(eraid));
            if(apukysely.next()) {
                if(!kohdennus.isEmpty())
                    kohdennus.append(" ");
                kohdennus.append( kp()->tositeTunnus(apukysely.value("tunniste").toInt(),
                                                     apukysely.value("pvm").toDate(),
                                                     apukysely.value("sarja").toString(),
                                                     kp()->tilikaudet()->tilikausiPaivalle(apukysely.value("pvm").toDate()).alkaa() == kp()->tilikaudet()->tilikausiPaivalle(pvm).alkaa() ));
            }
        }
    }
}

QVariant SelausRivi::data(int sarake, int role) const
{
    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (sarake)
        {
            case SelausModel::TOSITE:
                return role == Qt::EditRole ? vertailuTunnus : tositeTunnus;
            case SelausModel::PVM:
            if( role == Qt::DisplayRole)
                return pvm;
            else
                return QString("%1 %2").arg( pvm.toString(Qt::ISODate) ).arg( vientiId, 8, 10, QChar('0') );

            case SelausModel::TILI:
            {
                Tili *tiliO = kp()->tilit()->tili( tili );
                if( !tiliO )
                    return QVariant();
                if( role == Qt::EditRole)
                    return tiliO->numero();
                else if( tiliO->numero())
                    return tiliO->nimiNumero();
                else
                    return QVariant();
            }

            case SelausModel::DEBET:
            {
                if( role == Qt::EditRole)
                    return QVariant( debet );
                else if( qAbs(debet) > 1e-5 )
                    return QVariant( QString("%L1 €").arg( debet ,0,'f',2));
                else
                    return QVariant();
            }

            case SelausModel::KREDIT:
            {

                if( role == Qt::EditRole)
                    return QVariant( kredit);
                else if( qAbs(kredit) > 1e-5 )
                    return QVariant( QString("%L1 €").arg(kredit,0,'f',2));
                else
                    return QVariant();
            }
            case SelausModel::SELITE:
                if( selite == kumppani )
                    return QVariant();
                else
                    return selite;

            case SelausModel::KUMPPANI: return kumppani;

            case SelausModel::KOHDENNUS : return kohdennus;

        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( sarake==SelausModel::KREDIT || sarake == SelausModel::DEBET)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == Qt::UserRole)
    {
        // UserRolena on tositeid, jotta selauksesta pääsee helposti tositteeseen
        return tositeId;
    } else if( role == SelausModel::EtsiRooli) {
        return etsi;
    }
    else if( role == Qt::DecorationRole && sarake == SelausModel::KOHDENNUS )
    {
        if(maksettu)
            return QIcon(":/pic/ok.png");
        else if( kohdennustyyppi == Kohdennus::KUSTANNUSPAIKKA)
            return QIcon(":/pic/kohdennus.png");
        else if( kohdennustyyppi == Kohdennus::PROJEKTI )
            return QIcon(":/pic/projekti.png");
        else
            return QIcon(":/pic/tyhja.png");

    }
    else if( role == Qt::DecorationRole && sarake == SelausModel::PVM)
    {
        return kp()->tositeTyypit()->kuvake(tositeTyyppi);
    }
    else if( role == TositeSelausModel::TositeTyyppiRooli) {
        return tositeTyyppi;
    } else if(role == SelausModel::TiliRooli) {
        return tili;
    }


    return QVariant();
}
