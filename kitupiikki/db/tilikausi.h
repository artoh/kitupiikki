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

#ifndef TILIKAUSI_H
#define TILIKAUSI_H

#include <QDate>
#include <QDateTime>

#include "kantavariantti.h"

/**
 * @brief Yhden tilikauden tiedot
 */
class Tilikausi : public KantaVariantti
{
public:

    enum TilinpaatosTila
    {
        ALOITTAMATTA,
        KESKEN,
        VAHVISTETTU,
        EILAADITATILINAVAUKSELLE
    };

    enum Saannosto
    {
        MIKROYRITYS,
        PIENYRITYS,
        YRITYS
    };

    Tilikausi();    

    Tilikausi(QVariantMap data);
    Tilikausi(const QDate& alkaa, const QDate& paattyy);

    QDate alkaa() const { return alkaa_; }
    QDate paattyy() const { return paattyy_; }

    /**
     * @brief Milloin tämä tilikausi on viimeksi arkistoitu
     * @return
     */
    QDateTime arkistoitu();

    /**
     * @brief Milloin tämän tilikauden kirjauksia on viimeksi päivitetty
     * @return
     */
    QDateTime viimeinenPaivitys() const;

    QString kausivaliTekstina() const;

    /**
     * @brief Tilinpäätöksen laadinnan tila
     * @return
     */
    TilinpaatosTila tilinpaatoksenTila();


    /**
     * @brief Tilikauden yli/alijäämä
     * @return Tulos sentteinä
     */
    qlonglong tulos() const { return qRound64( dbl("tulos") * 100); }

    /**
     * @brief Tilikauden liikevaihto (CL-kirjaukset)
     * @return Liikevaihto sentteinä
     */
    qlonglong liikevaihto() const { return qRound64( dbl("liikevaihto") * 100); }

    /**
     * @brief Tilikauden päättävä tase
     * @return Tase sentteinä
     */
    qlonglong tase() const { return qRound64( dbl("tase") * 100); }

    /**
     * @brief Tilikauden keskimääräinen henkilöstö
     * @return
     */
    int henkilosto();

    /**
     * @brief Arkistohakemistossa käytettävä nimi
     * 
     * Nimi saadaan tilikauden alkupäivästä, niin että se on 
     * 2017 (vuosi), 2017-06 (kuukausi, jos ei 1), 2017-06-15 (jos päivä ei 1)
     * 
     * @return 
     */
    QString arkistoHakemistoNimi() const;

    /**
     * @brief Millä PMA-säännöstöllä tämän tilikauden puolesta saa toimia
     * @return
     */
    Saannosto pienuus();

    /**
     * @brief Kuinka moni pienen elinkeinonharjoittajan ehto ylittyy
     * @return
     */
    int pieniElinkeinonharjoittaja();


    /**
     * @brief Tilikauden lyhyt tunnus esim. 17, 17B
     * @return
     */
    QString kausitunnus() const { return kausitunnus_.mid(2);}
    QString pitkakausitunnus() const { return kausitunnus_; }

    void asetaKausitunnus(const QString& kausitunnus);


    void tallenna();
    void poista();

    QVariantMap data() const;

protected:
    QDate alkaa_;
    QDate paattyy_;

    QString kausitunnus_;
};

#endif // TILIKAUSI_H
