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

#ifndef RAPORTTIWIDGET_H
#define RAPORTTIWIDGET_H

#include <QObject>
#include <QPrinter>
#include <QWidget>
#include <QIcon>
#include <QPainter>


#include "raportinkirjoittaja.h"

class QCheckBox;
class QLabel;

/**
 * @brief Raportin kantaluokka
 *
 * Raporttikehys, jossa raportin esikatselu.
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
class RaporttiWidget : public QWidget
{
    Q_OBJECT
public:
    RaporttiWidget(QWidget *parent = nullptr);


signals:

public slots:
    /**
     * @brief Pdf-raportin esikatselu
     */
    virtual void esikatsele() = 0;

    virtual void nayta(RaportinKirjoittaja rk);    

    void esikatselu();    

protected:
    QWidget *raporttiWidget;
    QLabel* odotaLabel;

};

#endif // RAPORTTIWIDGET_H
