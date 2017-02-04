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

#include "vientimodel.h"
#include "kirjauswg.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include <QDebug>
#include <QSqlQuery>

VientiModel::VientiModel(KirjausWg *kwg) : kirjauswg(kwg)
{

}

int VientiModel::rowCount(const QModelIndex & /* parent */) const
{
    return viennit.count();
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
        VientiRivi rivi = viennit[index.row()];
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
    switch (index.column())
    {
    case PVM:      
        viennit[index.row()].pvm = value.toDate();
        emit siirryRuutuun( index.sibling(index.row(), TILI) );
        return true;
    case TILI:
    {
        Tili uusitili = Kirjanpito::db()->tili( value.toInt() );
        viennit[index.row()].tili = uusitili;
        qDebug() << uusitili.nimi() << "(" << uusitili.tyyppi() << ")";
        // Jos kirjataan tulotilille, niin siirrytään syöttämään kredit-summaa
        if( uusitili.tyyppi().startsWith('T'))
            emit siirryRuutuun(index.sibling(index.row(), KREDIT));
        else
            emit siirryRuutuun(index.sibling(index.row(), DEBET));
        return true;
    }
    case SELITE:
        viennit[index.row()].selite = value.toString();
        return true;
    case DEBET:
        viennit[index.row()].debetSnt = value.toInt();
        if(value.toInt())
        {
            viennit[index.row()].kreditSnt = 0;
            emit siirryRuutuun(index.sibling(index.row(), SELITE));
        }
        emit muuttunut();
        return true;
    case KREDIT:
        viennit[index.row()].kreditSnt = value.toInt();
        if( value.toInt())
        viennit[index.row()].debetSnt = 0;
        emit siirryRuutuun(index.sibling(index.row(), SELITE));
        emit muuttunut();
        return true;
    default:
        return false;
    }
}

Qt::ItemFlags VientiModel::flags(const QModelIndex &index) const
{
    if( muokkausSallittu)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index);
}

bool VientiModel::insertRows(int row, int count, const QModelIndex & /* parent */)
{
    beginInsertRows( QModelIndex(), row, row + count - 1);
    for(int i=0; i < count; i++)
        viennit.insert(row, uusiRivi() );
    endInsertRows();
    emit muuttunut();
    emit vientejaOnTaiEi(true);
    return true;
}

bool VientiModel::lisaaRivi()
{
    return insertRows( rowCount(QModelIndex()), 1, QModelIndex() );

}

int VientiModel::debetSumma() const
{
    int summa = 0;
    foreach (VientiRivi rivi, viennit)
    {
        summa += rivi.debetSnt;
    }
    return summa;
}

int VientiModel::kreditSumma() const
{
    int summa = 0;
    foreach (VientiRivi rivi, viennit)
    {
        summa += rivi.kreditSnt;
    }
    return summa;
}

void VientiModel::tallenna(int tositeid)
{
    QSqlQuery query;
    for(int i=0; i < viennit.count() ; i++)
    {
        VientiRivi rivi = viennit[i];
        if( rivi.vientiId )
        {
            query.prepare("UPDATE vienti SET pvm=:pvm, tili=:tili, debetsnt=:debetsnt, "
                          "kreditsnt=:kreditsnt, selite=:selite WHERE id=:id");
            query.bindValue(":id", rivi.vientiId);
        }
        else
            query.prepare("INSERT INTO vienti(tosite,pvm,tili,debetsnt,kreditsnt,selite) "
                                "VALUES(:tosite,:pvm,:tili,:debetsnt,:kreditsnt,:selite)");


        query.bindValue(":tosite", tositeid);
        query.bindValue(":pvm", rivi.pvm);
        query.bindValue(":tili", rivi.tili.id());
        query.bindValue(":debetsnt", rivi.debetSnt);
        query.bindValue(":kreditsnt", rivi.kreditSnt);
        query.bindValue(":selite", rivi.selite);
        query.exec();

        if( !rivi.vientiId )
            viennit[i].vientiId = query.lastInsertId().toInt();
    }
    // Lopuksi pitäisi vielä poistaa ne rivit, jotka on poistettu...
    // Tätä varten voisi ylläpitää poistettujen vientien Id-listaa
}

void VientiModel::tyhjaa()
{
    beginResetModel();
    viennit.clear();
    endResetModel();
    emit muuttunut();
}

void VientiModel::lataa(int tositeid)
{
    beginResetModel();
    viennit.clear();

    QSqlQuery query;
    query.exec(QString("SELECT id, pvm, tili, debetsnt, kreditsnt, selite FROM vienti WHERE tosite=%1 "
                       "ORDER BY id").arg(tositeid));
    while( query.next())
    {
        VientiRivi rivi;
        rivi.vientiId = query.value("id").toInt();
        rivi.pvm = query.value("pvm").toDate();
        rivi.tili = Kirjanpito::db()->tili( query.value("tili").toInt() );
        rivi.debetSnt = query.value("debetsnt").toInt();
        rivi.kreditSnt = query.value("kreditsnt").toInt();
        rivi.selite = query.value("selite").toString();
        viennit.append(rivi);
    }


    endResetModel();
}

void VientiModel::salliMuokkaus(bool sallitaanko)
{
    muokkausSallittu = sallitaanko;
}



VientiRivi VientiModel::uusiRivi()
{
    VientiRivi uusirivi;

    uusirivi.pvm = kirjauswg->tositePvm();

    int debetit = debetSumma();
    int kreditit = kreditSumma();

    // Täytetään automaattisesti, elleivät tilit vielä täsmää
    if( kreditit >= 0 && debetit >= 0 && kreditit != debetit)
    {
        if( kreditit > debetit)
            uusirivi.debetSnt = kreditit - debetit;
        else
            uusirivi.kreditSnt = debetit - kreditit;

        uusirivi.selite = viennit.last().selite;
    }

    return uusirivi;
}



