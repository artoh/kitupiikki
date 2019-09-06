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

#include "jsonkentta.h"

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
        YRITYS = 1,
        PIENYRITYS = 2,
        MIKROYRITYS = 4
    };

    Tilikausi();    

    Tilikausi(const QVariantMap& data);

    Tilikausi(QDate tkalkaa, QDate tkpaattyy, const QByteArray &json = QByteArray());


    QDate alkaa() const { return pvm("alkaa"); }
    QDate paattyy() const { return pvm("loppuu"); }

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

    JsonKentta *json() { return &json_; }

    /**
     * @brief Tilinpäätöksen laadinnan tila
     * @return
     */
    TilinpaatosTila tilinpaatoksenTila();


    /**
     * @brief Tilikauden yli/alijäämä
     * @return Tulos sentteinä
     */
    qlonglong tulos() const;

    /**
     * @brief Tilikauden liikevaihto (CL-kirjaukset)
     * @return Liikevaihto sentteinä
     */
    qlonglong liikevaihto() const;

    /**
     * @brief Tilikauden päättävä tase
     * @return Tase sentteinä
     */
    qlonglong tase() const;

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

    /**
     * @brief Onko tälle kaudelle laadittu budjettia
     * @return
     */
    bool onkoBudjettia();

protected:
    QDate alkaa_;
    QDate paattyy_;

    JsonKentta json_;

    QString kausitunnus_;
};

#endif // TILIKAUSI_H
