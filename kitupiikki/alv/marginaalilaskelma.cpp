/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "marginaalilaskelma.h"
#include "db/kirjanpito.h"
#include "db/verotyyppimodel.h"

#include <QSqlQuery>
#include <QMap>
#include <QSet>

MarginaaliLaskelmaRivi::MarginaaliLaskelmaRivi(int verokanta, qlonglong ostot, qlonglong myynnit, qlonglong alijaama) :
    verokanta_(verokanta), ostot_(ostot), myynnit_(myynnit), alijaama_(alijaama)
{

}

qlonglong MarginaaliLaskelmaRivi::marginaali() const
{
    return myynnit() - ostot() - alijaama();
}

qlonglong MarginaaliLaskelmaRivi::vero() const
{
    if( marginaali() <= 0)
        return 0;
    double dmarginaali = static_cast<double>(marginaali());
    double verosnt = ( dmarginaali / 100 ) * ( 1 - verokanta() / (100 + verokanta())) * verokanta();
    return qRound64( verosnt );
}


MarginaaliLaskelma::MarginaaliLaskelma(const QDate &alkaa, const QDate &loppuu)
{
    // Tekee verolaskelman: hakee ostojen ja myyntien summat verokausittain
    // sekä etsii edellisen alv-laskelman ja selvittää siitä, onko vähennettävää

    QSet<int> kannat;

    QSqlQuery query( *kp()->tietokanta() );

    // 1) Haetaan alijäämät
    QMap<int,qlonglong> alijaamat;
    query.exec( QString("select json from tosite where laji=0 and pvm=\"%1\"").arg(alkaa.addDays(-1).toString(Qt::ISODate)) );
    while( query.next() )
    {
        JsonKentta json( query.value("json").toByteArray() );
        QVariantMap varmap = json.variant("Voittomarginaalialijaama").toMap();
        QMapIterator<QString,QVariant> iter(varmap);
        while( iter.hasNext())
        {
            iter.next();
            kannat.insert( iter.key().toInt() );
            alijaamat.insert( iter.key().toInt(), iter.value().toLongLong());
        }
    }

    // 2) Haetaan myynnit
    QMap<int,qlonglong> myynnit;

    query.exec( QString("select alvprosentti, sum(kreditsnt) as plus, sum(debetsnt) as minus from vienti "
                        "where alvkoodi=%1 and pvm between \"%2\" and \"%3\" group by alvprosentti ")
                .arg(AlvKoodi::MYYNNIT_MARGINAALI).arg(alkaa.toString(Qt::ISODate)).arg(loppuu.toString(Qt::ISODate)));
    while( query.next())
    {
        kannat.insert( query.value("alvprosentti").toInt());
        myynnit.insert( query.value("alvprosentti").toInt(), query.value("plus").toLongLong() - query.value("minus").toLongLong());
    }

    // 3) Haetaan ostot
    QMap<int,qlonglong> ostot;

    query.exec( QString("select alvprosentti, sum(kreditsnt) as minus, sum(debetsnt) as plus from vienti "
                        "where alvkoodi=%1 and pvm between \"%2\" and \"%3\" group by alvprosentti ")
                .arg(AlvKoodi::OSTOT_MARGINAALI ).arg(alkaa.toString(Qt::ISODate)).arg(loppuu.toString(Qt::ISODate)));
    while( query.next())
    {
        kannat.insert( query.value("alvprosentti").toInt());
        ostot.insert( query.value("alvprosentti").toInt(), query.value("plus").toLongLong() - query.value("minus").toLongLong());
    }

    // 4) Tallennetaan

    for( int kanta : kannat)
    {
        rivit_.append( MarginaaliLaskelmaRivi(kanta, ostot.value(kanta), myynnit.value(kanta), alijaamat.value(kanta) ) );
    }


}

qlonglong MarginaaliLaskelma::vero() const
{
    qlonglong yhteensa = 0L;

    for( MarginaaliLaskelmaRivi rivi : rivit_)
        yhteensa += rivi.vero();

    return yhteensa;
}

qlonglong MarginaaliLaskelma::marginaali() const
{
    qlonglong yhteensa = 0l;
    for( MarginaaliLaskelmaRivi rivi : rivit_)
        yhteensa += rivi.marginaali();

    return yhteensa;
}

qlonglong MarginaaliLaskelma::vero(int kanta)
{
    for( MarginaaliLaskelmaRivi rivi : rivit_)
    {
        if( rivi.verokanta() == kanta)
            return rivi.vero();
    }
    return 0;
}

qlonglong MarginaaliLaskelma::myynnit(int kanta)
{
    for( MarginaaliLaskelmaRivi rivi : rivit_)
    {
        if( rivi.verokanta() == kanta)
            return rivi.myynnit();
    }
    return 0;
}

