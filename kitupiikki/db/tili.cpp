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

#include "tili.h"

#include <QDebug>
#include <QSqlQuery>

Tili::Tili() : id_(0), numero_(0), tila_(0), otsikkotaso_(0), muokattu_(false)
{

}

Tili::Tili(int id, int numero, const QString &nimi, const QString &tyyppi, int tila, int otsikkotaso) :
    id_(id), numero_(numero), nimi_(nimi), tyyppi_(tyyppi), tila_(tila), otsikkotaso_(otsikkotaso), muokattu_(false)

{

}

bool Tili::onkoValidi() const
{
    return numero() > 0 && !nimi().isEmpty();
}

int Tili::ysivertailuluku() const
{
    if( !otsikkotaso())
        return laskeysiluku( numero()) + 9; // Tavallinen tili
    else
        return laskeysiluku( numero() ) + otsikkotaso() - 1; // Otsikko
}

int Tili::kertymaPaivalle(const QDate &pvm)
{
    QString kysymys = QString("select sum(debetsnt), sum(kreditsnt) from vienti "
            " where tili = %1 and pvm <= \"%2\" ")
            .arg(numero())
            .arg(pvm.toString(Qt::ISODate));

    QSqlQuery kysely(kysymys);
    if( kysely.next())
    {
        int debetKertyma = kysely.value(0).toInt();
        int kreditKertyma = kysely.value(1).toInt();

        if( onkoTasetili() )
            return debetKertyma - kreditKertyma;
        else
            return kreditKertyma - debetKertyma;
    }
    return 0;
}

int Tili::montakoVientia() const
{
    QSqlQuery kysely( QString("SELECT sum(id) FROM vienti WHERE tili=%1").arg(id()) );
    if( kysely.next())
        return kysely.value(0).toInt();
    return 0;
}

bool Tili::onkoTasetili() const
{
    return( tyyppi().startsWith('A') || tyyppi().startsWith('B'));
}


bool Tili::onkoTulotili() const
{
    return tyyppi().startsWith('C');
}

bool Tili::onkoMenotili() const
{
    return tyyppi().startsWith('D');
}

bool Tili::onkoVastaavaaTili() const
{
    return tyyppi().startsWith('A');
}

bool Tili::onkoVastattavaaTili() const
{
    return tyyppi().startsWith('B');
}

int Tili::ysiluku(int luku, bool loppuu)
{
    if( loppuu )
        return laskeysiluku(luku + 1) - 1;
    else
        return laskeysiluku(luku);
}

int Tili::laskeysiluku(int luku)
{
    while( luku <= 99999999 )
        luku = luku * 10;
    return luku;
}


