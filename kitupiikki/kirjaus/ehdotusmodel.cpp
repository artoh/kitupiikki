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

#include "ehdotusmodel.h"
#include "db/kirjanpito.h"

EhdotusModel::EhdotusModel()
{

}

int EhdotusModel::rowCount(const QModelIndex & /* parent */) const
{
    return viennit_.count();
}

int EhdotusModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant EhdotusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case TILI:
                return QVariant(tr("Tili"));
            case DEBET :
                return QVariant(tr("Debet"));
            case KREDIT:
                return QVariant(tr("Kredit"));
        }

    }
    return QVariant();
}

QVariant EhdotusModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    VientiRivi rivi = viennit_.value( index.row() );

    if( role==Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {

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

void EhdotusModel::tyhjaa()
{
    beginResetModel();
    viennit_.clear();
    endResetModel();
}

void EhdotusModel::lisaaVienti(VientiRivi rivi)
{
    beginInsertRows( QModelIndex(), viennit_.count(), viennit_.count());
    viennit_.append(rivi);
    endInsertRows();
}

void EhdotusModel::tallenna(VientiModel *model, int yhdistettavaVastatiliNumero, QDate yhdistettavaPvm)
{
    qlonglong yhdistettavaDebet = 0;
    qlonglong yhdistettavaKredit = 0;

    foreach (VientiRivi vienti, viennit_) {
        if( yhdistettavaVastatiliNumero &&
            yhdistettavaVastatiliNumero == vienti.tili.numero() &&
            yhdistettavaPvm == vienti.pvm )
        {
            yhdistettavaDebet += vienti.debetSnt;
            yhdistettavaKredit += vienti.kreditSnt;
        }
        else
        {
            model->lisaaVienti(vienti);
        }
    }

    // #44 Erien yhdistäminen
    if( yhdistettavaDebet || yhdistettavaKredit )
    {
        for( int i=0; i < model->rowCount(QModelIndex()); i++)
        {
            QModelIndex indeksi = model->index(i, 0);
            if( indeksi.data(VientiModel::TiliNumeroRooli).toInt() == yhdistettavaVastatiliNumero &&
                indeksi.data(VientiModel::PvmRooli).toDate() == yhdistettavaPvm )
            {
                // Lisätään tämä alkuperäiseen erään
                model->setData(indeksi, indeksi.data(VientiModel::DebetRooli).toLongLong() + yhdistettavaDebet , VientiModel::DebetRooli );
                model->setData(indeksi, indeksi.data(VientiModel::KreditRooli).toLongLong() + yhdistettavaKredit, VientiModel::KreditRooli );
                break;
            }
        }
    }

}

struct maksuAlvEra
{
    maksuAlvEra() {}

    int alvprosentti = 0;
    qlonglong sentit = 0;
    int id = 0;
};

void EhdotusModel::viimeisteleMaksuperusteinen()
{
    for( VientiRivi rivi : viennit_)
    {
        if( kp()->onkoMaksuperusteinenAlv(rivi.pvm) &&
                (rivi.tili.onko(TiliLaji::MYYNTISAATAVA) || rivi.tili.onko(TiliLaji::OSTOVELKA)))
        {
            bool myynti = rivi.tili.onko(TiliLaji::MYYNTISAATAVA);
            Tili haeTili = myynti ? kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA) :
                                                              kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA);

            // parissa veroprosentti,debet-kredit
            QList<maksuAlvEra> verot;
            qlonglong verosumma = 0;

            TaseEra era(rivi.eraId);
            QSqlQuery tositeKysely( QString("SELECT tosite FROM vienti WHERE id=%1").arg(rivi.eraId));
            int tosite = tositeKysely.next() ? tositeKysely.value("tosite").toInt() : -1;


            // Haetaan tositteella olevat kohdentamattomat verokirjaukset

            QSqlQuery kysely( QString("SELECT alvprosentti, debetsnt, kreditsnt, id FROM vienti "
                                      "WHERE tili=%1 AND tosite=%2").arg(haeTili.id()).arg(tosite));
            while( kysely.next())
            {
                maksuAlvEra maksuEra;
                maksuEra.alvprosentti = kysely.value("alvprosentti").toInt();
                maksuEra.sentit = kysely.value("debetsnt").toLongLong() - kysely.value("kreditsnt").toLongLong();
                maksuEra.id = kysely.value("id").toInt();
                verot.append(maksuEra);
                verosumma += maksuEra.sentit;
            }


            double kerroin = 1.00;
            if( era.saldoSnt != rivi.debetSnt - rivi.kreditSnt )
                kerroin = ((double) (rivi.debetSnt - rivi.kreditSnt)) / (double) era.saldoSnt;

            // Nyt sitten tehdään suhteelliset kirjaukset
            for( auto vero : verot)
            {
                qlonglong sentit = kerroin == 1.00 ?  vero.sentit :
                                                      qRound( kerroin * (double) vero.sentit );

                // Kirjataan alv-velkaan taikka alv-saataviin
                VientiRivi verorivi;
                verorivi.pvm = rivi.pvm;
                verorivi.tili = myynti ? kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA) :
                                         kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA);

                verorivi.debetSnt = sentit < 0 ? 0 - sentit : 0;
                verorivi.kreditSnt = sentit > 0 ? sentit : 0;
                verorivi.alvprosentti = vero.alvprosentti;
                verorivi.selite = tr("Maksuperusteinen %1 % alv %2 / %3 [%4]").arg(verorivi.alvprosentti)
                        .arg(era.tositteenTunniste()).arg(era.pvm.toString(Qt::SystemLocaleShortDate))
                        .arg(era.selite);

                verorivi.alvkoodi = myynti ? AlvKoodi::ALVKIRJAUS + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI :
                                             AlvKoodi::ALVVAHENNYS + AlvKoodi::MAKSUPERUSTEINEN_OSTO ;

                lisaaVienti(verorivi);

                // Rivi, jolla kirjataan pois kohdentamattoman veron tililtä
                VientiRivi poisrivi;
                poisrivi.tili = haeTili;
                poisrivi.pvm = rivi.pvm;
                poisrivi.debetSnt = sentit > 0 ? sentit : 0;
                poisrivi.kreditSnt = sentit < 0 ? 0 - sentit : 0;
                poisrivi.selite = verorivi.selite;
                poisrivi.eraId = vero.id;
                lisaaVienti(poisrivi);

            }

        }
    }
}

bool EhdotusModel::onkoKelpo(bool toispuolinen) const
{
    int debetSumma = 0;
    int kreditSumma = 0;

    foreach (VientiRivi rivi, viennit_)
    {
        debetSumma += rivi.debetSnt;
        kreditSumma += rivi.kreditSnt;
    }

    return ( debetSumma > 0 && ( debetSumma == kreditSumma || toispuolinen ));
}
