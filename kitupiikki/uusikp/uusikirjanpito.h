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

#ifndef UUSIKIRJANPITO_H
#define UUSIKIRJANPITO_H

#include <QWizard>
#include <QMap>

#include "ui_intro.h"

/**
  * @dir uusikp
  * @brief Uuden kirjanpidon luomiseen ja tilikartan päivitykseen liittyvät luokat
  *
  * Uusi kirjanpito luodaan UusiKirjanpito::aloitaUusiKirjanpito()-funktiolla
  *
  */



/**
 * @brief Uuden kirjanpidon luominen
 *
 * Uusi kirjanpito luodaan aloitaUusiKirjanpito()-funktiolla, joka näyttää
 * valintavelhon ja luo tarvittavat tiedostot.
 *
 *  sqlite-tietokannan alustamiskomennot ovat luo.sql-tiedostossa
 *
 */
class UusiKirjanpito : public QWizard
{
    Q_OBJECT

protected:
    UusiKirjanpito();

public:
    enum { INTROSIVU = 1, TILIKARTTASIVU = 2, NIMISIVU = 3, TILIKAUSISIVU = 4,
           KIRJAUSPERUSTESIVU = 5, SIJAINTISIVU = 6, LOPPUSIVU = 7};


    /**
     * @brief Näyttää valintavelhon ja luo uuden kirjanpidon
     * @return Polku uuden kirjanpidon hakemistoon tai String(), ellei luominen onnistunut
     */
    static QString aloitaUusiKirjanpito();

    /**
     * @brief Lukee Kitupiikin tilikartta -muotoisen tiedoston
     * @param Tiedostonpolku
     * @return Tiedoston sisältö. QMap:issa avaimena osion nimi esim. [kuvaus] ja sisältö QStringList-muodossa
     */
    static QMap<QString,QStringList> lueKtkTiedosto(const QString& polku);


protected slots:
    /**
     * @brief Näyttää selaimessa ohjeen
     */
    void naytaOhje();

protected:
    /**
     * @brief Luo hakemistorakenteen ja tietokannan
     * @return Tosi jos onnistui
     */
    bool alustaKirjanpito();

};

#endif // UUSIKIRJANPITO_H
