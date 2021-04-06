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
            return tr("Tosite");
        case PVM :
            return tr("Pvm");
        case TILI:
            return tr("Tili");
        case DEBET :
            return tr("Debet");
        case KREDIT:
            return tr("Kredit");
        case KOHDENNUS:
            return tr("Kohdennus");
        case KUMPPANI:
            return tr("Asiakas/Toimittaja");
        case SELITE:
            return tr("Selite");
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

void SelausModel::lataaSqlite(SQLiteModel* sqlite, const QDate &alkaa, const QDate &loppuu, int tili)
{

    QString tilistr = tili ? QString(" AND tili=%1").arg(tili) : "";

    QString kysymys = QString("SELECT vienti.id AS id, vienti.pvm as pvm, vienti.tili as tili, debetsnt, kreditsnt, "
                    "selite, vienti.kohdennus as kohdennus, eraid, vienti.tosite as tosite_id, tosite.pvm as tosite_pvm, tosite.tunniste as tosite_tunniste,"
                    "tosite.tyyppi as tosite_tyyppi, tosite.sarja as tosite_sarja, "
                    "kumppani.nimi as kumppani_nimi, liitteita "
                    "FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                    "LEFT OUTER JOIN Kumppani ON Vienti.kumppani=kumppani.id "
                    "LEFT OUTER JOIN (SELECT tosite, COUNT(id) AS liitteita FROM Liite GROUP BY tosite) AS lq ON Tosite.id=lq.tosite "
                    "WHERE tila >= 100 AND Vienti.pvm BETWEEN '%1' AND '%2' %3 ORDER BY pvm")
            .arg(alkaa.toString(Qt::ISODate))
            .arg(loppuu.toString(Qt::ISODate))
            .arg(tilistr);

    beginResetModel();

    kaytetytTilit_.clear();
    rivit_.clear();

    QSqlQuery kysely( sqlite->tietokanta() );
    kysely.exec(kysymys);

    bool merkkauksia = kp()->kohdennukset()->merkkauksia();

    while(kysely.next()) {
        SelausRivi rivi(kysely, samakausi_, sqlite, merkkauksia);
        rivit_.append(rivi);
        kaytetytTilit_.insert(rivi.getTili());
    }


    endResetModel();

}

int SelausModel::tili(int rivi) const
{
    return rivit_[rivi].getTili();
}

QString SelausModel::etsiTeksti(int rivi) const
{
    return rivit_[rivi].getEtsi();
}

QList<int> SelausModel::tiliLista() const
{
    QList<int> lista = kaytetytTilit_.toList();
    std::sort(lista.begin(), lista.end());
    return lista;
}


void SelausModel::lataa(const QDate &alkaa, const QDate &loppuu, int tili)
{
    samakausi_ = kp()->tilikausiPaivalle(alkaa).alkaa() == kp()->tilikausiPaivalle(loppuu).alkaa();
    tiliselaus_ = tili;

    if( kp()->yhteysModel()) {

        SQLiteModel* sqlite = qobject_cast<SQLiteModel*>( kp()->yhteysModel() );
        if( sqlite ) {
            lataaSqlite(sqlite, alkaa, loppuu, tili);
        } else if( !ladataan_){
            KpKysely *kysely = kpk("/viennit");
            if(kysely) {
                kysely->lisaaAttribuutti("alkupvm", alkaa);
                kysely->lisaaAttribuutti("loppupvm", loppuu);
                if(tili)
                    kysely->lisaaAttribuutti("tili", tili);
                connect( kysely, &KpKysely::vastaus, this, &SelausModel::tietoSaapuu);
                ladataan_ = true;

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
    ladataan_ = false;
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
    debet = data.value("debet").toString();
    kredit = data.value("kredit").toString();
    kumppani = data.value("kumppani").toMap().value("nimi").toString();
    selite = data.value("selite").toString();
    liitteita = data.value("liitteita").toInt();

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

SelausRivi::SelausRivi(QSqlQuery &data, bool samakausi, SQLiteModel *sqlite, bool merkkauksia)
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
    debet = data.value("debetsnt").toLongLong();
    kredit = data.value("kreditsnt").toLongLong();
    kumppani = data.value("kumppani_nimi").toString();
    selite = data.value("selite").toString();
    liitteita = data.value("liitteita").toInt();

    etsi = tositeTunnus + " " + kumppani + " " + selite;
    maksettu = false;

    Kohdennus kohdennusObj = kp()->kohdennukset()->kohdennus(data.value("kohdennus").toInt());
    kohdennustyyppi = kohdennusObj.tyyppi();
    if(kohdennustyyppi)
       kohdennus = kohdennusObj.nimi();


    QSqlQuery apukysely( sqlite->tietokanta());
    if(merkkauksia) {

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
                if( role==Qt::EditRole) {
                    return tili;
                } else if(tili) {
                    return QString("%1 %2").arg(tili).arg(kp()->tilit()->nimi(tili));
                } else {
                    return QVariant();
                }

            }

            case SelausModel::DEBET:
            {
                if( role == Qt::EditRole)
                    return debet.cents();
                else
                    return debet.display(false);
            }

            case SelausModel::KREDIT:
            {

                if( role == Qt::EditRole)
                    return kredit.cents();
                else
                    return kredit.display(false);
            }
            case SelausModel::SELITE:
                if( selite == kumppani )
                    return QVariant();
                else
                    return selite;

            case SelausModel::KUMPPANI: return kumppani;

            case SelausModel::KOHDENNUS : return kohdennus;

        }
    } else if( role == Qt::TextAlignmentRole)
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
    else if( role == Qt::DecorationRole && sarake==SelausModel::TOSITE )
    {
        if(  liitteita )
            return QIcon(":/pic/liite.png");
        else
            return QIcon(":/pic/tyhja.png");
    }
    else if( role == TositeSelausModel::TositeTyyppiRooli) {
        return tositeTyyppi;
    } else if(role == SelausModel::TiliRooli) {
        return tili;
    }


    return QVariant();
}
