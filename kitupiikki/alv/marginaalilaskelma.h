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
#ifndef MARGINAALILASKELMA_H
#define MARGINAALILASKELMA_H

#include <QList>

/**
 * @brief Yhden verokannan marginaaliverolaskelma
 */
class MarginaaliLaskelmaRivi
{
public:
    MarginaaliLaskelmaRivi(int verokanta, qlonglong ostot, qlonglong myynnit, qlonglong alijaama=0);

    int verokanta() const { return verokanta_;}
    qlonglong ostot() const { return  ostot_;}
    qlonglong myynnit() const { return myynnit_;}
    qlonglong alijaama() const { return alijaama_;}

    qlonglong marginaali() const;
    qlonglong vero() const;

private:
    int verokanta_;
    qlonglong ostot_;
    qlonglong myynnit_;
    qlonglong alijaama_;
};


/**
 * @brief Voittomarginaaliveron laskelma
 */
class MarginaaliLaskelma
{
public:
    MarginaaliLaskelma(const QDate& alkaa, const QDate& loppuu);

    int riveja() const { return rivit_.count();}
    MarginaaliLaskelmaRivi rivi(int indeksi) { return rivit_.at(indeksi);}    
    qlonglong vero() const;
    qlonglong marginaali() const;

    qlonglong vero(int kanta);
    qlonglong myynnit(int kanta);

protected:
    QList<MarginaaliLaskelmaRivi> rivit_;
};

#endif // MARGINAALILASKELMA_H
