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

#ifndef MUOKATTAVARAPORTTI_H
#define MUOKATTAVARAPORTTI_H

#include <QDate>
#include <QMap>
#include <QVector>

#include "raportti.h"
#include "ui_muokattavaraportti.h"

/**
 * @brief Muokattavan raportin summatiedot yhdestä kaudesta
 */
struct RaporttiData
{
    RaporttiData()  {;}
    RaporttiData(QDate alkaaPvm, QDate paattyyPvm);

    QDate alkaa;        // Raporttikauden alkupäivä
    QDate paattyy;      // Raporttikauden loppupäivä TAI tasepäivä

    QMap<int,int> summat;   // summat <ysiluku,sentit>
};



/**
 * @brief Muokattavien Tase/Tulosraporttien tulostaminen
 *
 */
class MuokattavaRaportti : public Raportti
{
public:
    MuokattavaRaportti(const QString& raporttinimi, QPrinter *printer);
    ~MuokattavaRaportti();

    RaportinKirjoittaja raportti();

    /**
     * @brief Palauttaa raportin annetulla datalla
     * @param raporttiData Aikamäärittely datassa
     * @param taseRaportti Onko taseraportti (vai tulosraportti)
     * @return RaportinKirjoittaja, johon raportti kirjoitettu
     *
     * Tätä käytetään kirjoitettaessa esim. arkistoinnin yhteydessä
     * raporttia ilman, että graafisia valintoja näytetään.
     *
     */
    RaportinKirjoittaja raporttiDatalla( QVector<RaporttiData> raporttiData,
                                         bool taseRaportti = false);
protected:
    void alustaData();
    RaportinKirjoittaja kirjoitaRaportti();
    void kirjoitaYlatunnisteet(RaportinKirjoittaja *rk);
    void laskeTulosData();
    void laskeTaseData();

protected:
    Ui::MuokattavaRaportti *ui;
    QString otsikko;
    QStringList kaava;
    QString optiorivi;
    bool tase;      // Koskeeko raportti tasetta
    bool tulos;     // Koskeeko raportti tulosta

    QVector<RaporttiData> data;
    QMap<int,int> tilisummat;
};

#endif // MUOKATTAVARAPORTTI_H
