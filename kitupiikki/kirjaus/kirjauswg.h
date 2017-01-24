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

#ifndef KIRJAUSWG_H
#define KIRJAUSWG_H

#include <QWidget>

#include "ui_kirjaus.h"
#include "tositewg.h"

class VientiModel;


class Kirjanpito;

class KirjausWg : public QWidget
{
    Q_OBJECT
public:
    KirjausWg(Kirjanpito *kp, TositeWg *tosite);
    ~KirjausWg();

    QDate tositePvm() const;

signals:

public slots:
    void lisaaRivi();
    void tyhjenna();
    void tallenna();
    void naytaSummat();
    void lataaTosite(int id);
    void paivitaKommenttiMerkki();

protected:
    Ui::KirjausWg *ui;
    VientiModel *viennitModel;
    Kirjanpito *kirjanpito;
    TositeWg *tositewg;

    int tositeId;   /** Käsiteltävänä olevan tositteen id tai 0 jos tositetta ei tallennettu */
};

#endif // KIRJAUSWG_H
