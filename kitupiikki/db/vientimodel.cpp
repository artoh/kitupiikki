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

#include "db/vientimodel.h"
#include "db/tositemodel.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"



#include <QDebug>
#include <QSqlQuery>

VientiModel::VientiModel(TositeModel *tositemodel) : tositeModel_(tositemodel), muokattu_(false)
{

}

int VientiModel::rowCount(const QModelIndex & /* parent */) const
{
    return viennit_.count();
}

int VientiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 7;
}

QVariant VientiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case PVM:
                return QVariant("Pvm");
            case TILI:
                return QVariant("Tili");
            case DEBET :
                return QVariant("Debet");
            case KREDIT:
                return QVariant("Kredit");
            case SELITE:
                return QVariant("Selite");
            case KUSTANNUSPAIKKA :
                return QVariant("Kustannuspaikka");
            case PROJEKTI:
                return QVariant("Projekti");
        }

    }
    return QVariant( section + 1);
}

QVariant VientiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    if( role==Qt::DisplayRole || role == Qt::EditRole)
    {
        VientiRivi rivi = viennit_[index.row()];
        switch (index.column())
        {
            case PVM: return QVariant( rivi.pvm );

            case TILI:
                if( rivi.tili.numero())
                    return QVariant( QString("%1 %2").arg(rivi.tili.numero()).arg(rivi.tili.nimi()) );
                else
                    return QVariant();

            case DEBET:
                if( role == Qt::EditRole)
                    return QVariant( rivi.debetSnt);
                else if( rivi.debetSnt )
                    return QVariant( QString("%L1 €").arg(rivi.debetSnt / 100.0,0,'f',2));
                else
                    return QVariant();

            case KREDIT:
                if( role == Qt::EditRole)
                    return QVariant( rivi.kreditSnt);
                else if( rivi.kreditSnt )
                    return QVariant( QString("%L1 €").arg(rivi.kreditSnt / 100.0,0,'f',2));
                else
                    return QVariant();

            case SELITE: return QVariant( rivi.selite );
            case KUSTANNUSPAIKKA: return QVariant( "");
            case PROJEKTI: return QVariant("");
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==KREDIT || index.column() == DEBET)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    return QVariant();
}

bool VientiModel::setData(const QModelIndex &index, const QVariant &value, int /* role */)
{
    muokattu_ = true;

    switch (index.column())
    {
    case PVM:
        viennit_[index.row()].pvm = value.toDate();
        emit siirryRuutuun( index.sibling(index.row(), TILI) );
        return true;
    case TILI:
    {
        // Tili asetetaan numerolla!
        Tili uusitili = Kirjanpito::db()->tilit()->tiliNumerolla( value.toInt());
        viennit_[index.row()].tili = uusitili;
        qDebug() << uusitili.nimi() << "(" << uusitili.tyyppi() << ")" << value.toInt();
        // Jos kirjataan tulotilille, niin siirrytään syöttämään kredit-summaa
        if( uusitili.tyyppi().startsWith('T'))
            emit siirryRuutuun(index.sibling(index.row(), KREDIT));
        else
            emit siirryRuutuun(index.sibling(index.row(), DEBET));
        return true;
    }
    case SELITE:
        viennit_[index.row()].selite = value.toString();
        return true;
    case DEBET:
        viennit_[index.row()].debetSnt = value.toInt();
        if(value.toInt())
        {
            viennit_[index.row()].kreditSnt = 0;
            emit siirryRuutuun(index.sibling(index.row(), SELITE));
        }
        emit muuttunut();
        return true;
    case KREDIT:
        viennit_[index.row()].kreditSnt = value.toInt();
        if( value.toInt())
        viennit_[index.row()].debetSnt = 0;
        emit siirryRuutuun(index.sibling(index.row(), SELITE));
        emit muuttunut();
        return true;
    default:
        return false;
    }
}

Qt::ItemFlags VientiModel::flags(const QModelIndex &index) const
{
    // TODO: Onko muokkaus sallittu
    if( false )
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index);
}

bool VientiModel::insertRows(int row, int count, const QModelIndex & /* parent */)
{
    beginInsertRows( QModelIndex(), row, row + count - 1);
    for(int i=0; i < count; i++)
        viennit_.insert(row, uusiRivi() );
    endInsertRows();
    emit muuttunut();
    // emit vientejaOnTaiEi(true);
    return true;
}

bool VientiModel::lisaaRivi()
{
    return insertRows( rowCount(QModelIndex()), 1, QModelIndex() );

}

bool VientiModel::lisaaVienti(const QDate &pvm, int tilinumero, const QString &selite, int debetSnt, int kreditSnt)
{
    VientiRivi uusi;
    uusi.pvm = pvm;
    uusi.tili = kp()->tilit()->tiliNumerolla(tilinumero);
    uusi.selite = selite;
    uusi.debetSnt = debetSnt;
    uusi.kreditSnt = kreditSnt;

    beginInsertRows( QModelIndex(), viennit_.count(), viennit_.count());
    viennit_.append( uusi );
    endInsertRows();

    return true;
}

int VientiModel::debetSumma() const
{
    int summa = 0;
    foreach (VientiRivi rivi, viennit_)
    {
        summa += rivi.debetSnt;
    }
    return summa;
}

int VientiModel::kreditSumma() const
{
    int summa = 0;
    foreach (VientiRivi rivi, viennit_)
    {
        summa += rivi.kreditSnt;
    }
    return summa;
}

void VientiModel::tallenna()
{
    QSqlQuery query(*tositeModel_->tietokanta());
    for(int i=0; i < viennit_.count() ; i++)
    {
        VientiRivi rivi = viennit_[i];
        if( rivi.vientiId )
        {
            query.prepare("UPDATE vienti SET pvm=:pvm, tili=:tili, debetsnt=:debetsnt, "
                          "kreditsnt=:kreditsnt, selite=:selite, alvkoodi=:alvkoodi,"
                          "alvprosentti=:alvprosentti, muokattu=:muokattu, json=:json WHERE id=:id");
            query.bindValue(":id", rivi.vientiId);
        }
        else
        {
            query.prepare("INSERT INTO vienti(tosite,pvm,tili,debetsnt,kreditsnt,selite,"
                           "alvkoodi, luotu, muokattu) "
                            "VALUES(:tosite,:pvm,:tili,:debetsnt,:kreditsnt,:selite,"
                            ":alvkoodi, :luotu, :muokattu, json=:json)");
            query.bindValue(":luotu", QVariant( QDateTime(kp()->paivamaara(), QTime::currentTime() ) ) );
        }


        query.bindValue(":tosite", tositeModel_->id() );
        query.bindValue(":pvm", rivi.pvm);
        query.bindValue(":tili", rivi.tili.id());
        query.bindValue(":debetsnt", rivi.debetSnt);
        query.bindValue(":kreditsnt", rivi.kreditSnt);
        query.bindValue(":selite", rivi.selite);
        query.bindValue(":alvkoodi", rivi.alvkoodi);
        query.bindValue(":alvprosentti", rivi.alvprosentti);
        query.bindValue(":muokattu", QVariant( QDateTime(kp()->paivamaara(), QTime::currentTime() ) ) );
        query.bindValue(":json", rivi.json.toSqlJson());
        query.exec();

        if( !rivi.vientiId )
            viennit_[i].vientiId = query.lastInsertId().toInt();
    }
    // Lopuksi pitäisi vielä poistaa ne rivit, jotka on poistettu...
    // Tätä varten voisi ylläpitää poistettujen vientien Id-listaa

    muokattu_ = false;
}

void VientiModel::tyhjaa()
{
    beginResetModel();
    viennit_.clear();
    muokattu_ = false;
    endResetModel();
}

void VientiModel::lataa()
{
    beginResetModel();
    viennit_.clear();

    QSqlQuery query( *tositeModel_->tietokanta() );
    query.exec(QString("SELECT id, pvm, tili, debetsnt, kreditsnt, selite, "
                       "alvkoodi, alvprosentti, luotu, muokattu, json "
                       "FROM vienti WHERE tosite=%1 "
                       "ORDER BY id").arg( tositeModel_->id() ));
    while( query.next())
    {
        VientiRivi rivi;
        rivi.vientiId = query.value("id").toInt();
        rivi.pvm = query.value("pvm").toDate();
        rivi.tili = Kirjanpito::db()->tilit()->tiliIdlla( query.value("tili").toInt());
        rivi.debetSnt = query.value("debetsnt").toInt();
        rivi.kreditSnt = query.value("kreditsnt").toInt();
        rivi.selite = query.value("selite").toString();
        rivi.alvkoodi = query.value("alvkoodi").toInt();
        rivi.alvprosentti = query.value("alvprosentti").toInt();
        rivi.luotu = query.value("luotu").toDateTime();
        rivi.muokattu = query.value("muokattu").toDateTime();
        rivi.json.fromJson( query.value("json").toByteArray() );
        viennit_.append(rivi);
    }


    endResetModel();
}



VientiRivi VientiModel::uusiRivi()
{
    VientiRivi uusirivi;

    uusirivi.pvm = tositeModel_->pvm();

    int debetit = debetSumma();
    int kreditit = kreditSumma();

    // Täytetään automaattisesti, elleivät tilit vielä täsmää
    if( kreditit >= 0 && debetit >= 0 && kreditit != debetit)
    {
        if( kreditit > debetit)
            uusirivi.debetSnt = kreditit - debetit;
        else
            uusirivi.kreditSnt = debetit - kreditit;

        uusirivi.selite = viennit_.last().selite;
    }

    return uusirivi;
}



