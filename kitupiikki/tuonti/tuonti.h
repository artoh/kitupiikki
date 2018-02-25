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


/**
  * @dir tuonti
  * @brief Tiedostojen tuonti kirjanpitoon
  *
  */



#ifndef TUONTI_H
#define TUONTI_H

#include <QString>
#include <QObject>

#include "db/tositelajimodel.h"
#include "db/tili.h"

class KirjausWg;

/**
 * @brief Tiedostojen tuonti
 */
class Tuonti : public QObject
{
    Q_OBJECT
protected:
    Tuonti(KirjausWg *wg);

public:
    /**
     * @brief Tuo tiedoston
     * @param tiedostonnimi Tiedoston nimi
     * @return tosi, jos tiedosto lisätään myös liitteeksi
     */
    virtual bool tuoTiedosto(const QString& tiedostonnimi) = 0;

    /**
     * @brief Yrittää tuoda halutun tiedoston
     * @param tiedostonnimi Polku tiedostoon
     * @param wg KirjausWg, johon tuodaan
     * @return tosi, jos tiedosto lisätään
     */
    static bool tuo(const QString& tiedostonnimi, KirjausWg *wg);

protected:
    KirjausWg* kirjausWg() { return kirjausWg_; }

    /**
     * @brief Tuo laskun
     * @param sentit Laskun summa sentteinä
     * @param laskupvm Laskun päivämäärä
     * @param toimituspvm Laskulla oleva toimituspäivä
     * @param erapvm Laskun eräpäivä
     * @param viite Laskun viite
     * @param tilinumero Saajan/Maksajan IBAN-tilinumero
     * @param saajanNimi Saajan/Maksajan nimi
     */
    void tuoLasku(qlonglong sentit,
                    QDate laskupvm, QDate toimituspvm, QDate erapvm, QString viite,
                    QString tilinumero, QString saajanNimi);

    /**
     * @brief Aloittaa tiliotteen tuomisen
     * @param iban Tilin IBAN-numero
     * @param mista Tiliote alkaa
     * @param mihin Tiliote päättyy
     * @return tosi, jos onnistuu (tili olemassa)
     */
    bool tiliote(QString iban, QDate mista, QDate mihin);

    Tili tiliotetili() const { return tiliotetili_; }

    KirjausWg* kirjausWg_;
    Tili tiliotetili_;
};

#endif // TUONTI_H
