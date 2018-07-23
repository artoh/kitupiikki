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

#include <QSettings>
#include "raporttirivi.h"


RaporttiRivi::RaporttiRivi(RivinKaytto kaytto)
    : lihava_(false), ylaviiva_(false), pistekoko_(10), rivinKaytto_(kaytto)
{

}

void RaporttiRivi::lisaa(const QString &teksti, int sarakkeet, bool tasaaOikealle, RaporttiRiviSarake::SarakkeenKaytto kaytto)
{
    RaporttiRiviSarake uusi;
    uusi.arvo = QVariant(teksti);

    uusi.leveysSaraketta = sarakkeet;
    uusi.tasaaOikealle = tasaaOikealle;
    uusi.kaytto = kaytto;
    sarakkeet_.append(uusi);
}

void RaporttiRivi::lisaaLinkilla(RaporttiRiviSarake::Linkki linkkityyppi, int linkkitieto, const QString &teksti, int sarakkeet, RaporttiRiviSarake::SarakkeenKaytto kaytto)
{
    RaporttiRiviSarake uusi;
    uusi.arvo = QVariant( teksti );

    uusi.leveysSaraketta = sarakkeet;
    uusi.linkkityyppi = linkkityyppi;
    uusi.linkkidata = linkkitieto;
    uusi.kaytto = kaytto;
    sarakkeet_.append(uusi);
}

void RaporttiRivi::lisaa(qlonglong sentit, bool tulostanollat,RaporttiRiviSarake::SarakkeenKaytto kaytto)
{
    RaporttiRiviSarake uusi;
    if( sentit || tulostanollat)
        uusi.arvo = QVariant(sentit);

    uusi.tasaaOikealle = true;
    uusi.kaytto = kaytto;
    sarakkeet_.append( uusi );
}

void RaporttiRivi::lisaa(const QDate &pvm, RaporttiRiviSarake::SarakkeenKaytto kaytto)
{
    lisaa( pvm.toString("dd.MM.yyyy"), 1, false, kaytto);
}

QString RaporttiRivi::teksti(int sarake)
{
    QVariant arvo = sarakkeet_.at(sarake).arvo;

    if( arvo.type() == QVariant::LongLong )
    {
        return QString("%L1").arg(  arvo.toLongLong()  / 100.0 ,0,'f',2 );
    }
    else if( arvo.type() == QVariant::Date )
    {
        return arvo.toDate().toString("dd.MM.yyyy");
    }
    else if( arvo.type() == QVariant::String)
    {
        return arvo.toString();
    }
    else
        return QString();

}

QString RaporttiRivi::csv(int sarake)
{
    QVariant arvo = sarakkeet_.at(sarake).arvo;
    QSettings settings;

    if( arvo.type() == QVariant::LongLong )
    {
        QChar despilkku = settings.value("CsvDesimaali", QChar(',')).toChar();
        if( despilkku == ',')
            return QString("\"%1\"").arg( arvo.toLongLong()  / 100.0 ,0,'f',2 ).replace('.',',');
        else
            return QString("\"%1\"").arg( arvo.toLongLong()  / 100.0 ,0,'f',2 );
    }
    else if( arvo.type() == QVariant::Date )
    {
        QString pvmmuoto = settings.value("CsvPaivays", "dd.MM.yyyy").toString();
        return arvo.toDate().toString(pvmmuoto);
    }
    else if( arvo.type() == QVariant::String)
    {
        QString str = arvo.toString();
        if( str.contains(',') || str.contains('\"') || str.contains('\n') || str.contains(';') || str.contains(' ') || str.contains('\t'))
        {
            str.replace("\"", "\"\"");
            return QString("\"%1\"").arg(str);
        }
        return arvo.toString();
    }
    else
        return QString();
}


