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

#ifndef RAPORTTI_H
#define RAPORTTI_H

#include <QObject>
#include <QPrinter>
#include <QWidget>
#include <QIcon>
#include <QPainter>


#include "raportinkirjoittaja.h"

class QCheckBox;

/**
 * @brief Raportin kantaluokka
 *
 * Raporttikehys, jossa raportin tulostus ja esikatselu.
 * Tästä periytetty raporttiluokka luo oman käyttöliittymänsä
 * raporttiWidget -widgetin sisään
 *
 * @code
 * ui->setupUi( raporttiWidget );
 * @endcode
 *
 * Lisäksi periytetyllä raportilla on Raportti-funktio, joka palauttaa
 * RaportinKirjoittaja-olion, johon raportti on kirjoitettu.
 *
 */
class Raportti : public QWidget
{
    Q_OBJECT
public:
    Raportti(QWidget *parent = 0);


    /**
     * @brief Palauttaa pyydetyn raportin
     * @return RaportinKirjoittaja, jonne raportti on kirjoitettu
     */
    virtual RaportinKirjoittaja raportti() = 0;


signals:

public slots:
    /**
     * @brief Tulostaa raportin
     */
    void tulosta();
    /**
     * @brief Pdf-raportin esikatselu
     */
    void esikatsele();

    /**
     * @brief Avaa html-muotoisena selaimessa
     */
    void avaaHtml();

    /**
     * @brief Näyttää dialogin sivun asetuksista
     */
    void sivunAsetukset();

    /**
     * @brief Kopioi raportin leikepöydälle
     */
    void leikepoydalle();

protected:
    QWidget *raporttiWidget;
    QCheckBox *raitaCheck;


};

#endif // RAPORTTI_H
