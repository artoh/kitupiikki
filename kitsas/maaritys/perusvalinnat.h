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

#ifndef PERUSVALINNAT_H
#define PERUSVALINNAT_H

#include <QWidget>
#include <QImage>

#include "tallentavamaarityswidget.h"

namespace Ui {
class Perusvalinnat;
}

/**
 * @brief Määrityswidget perusvalinnoille (nimi, y-tunnus, logo)
 */
class Perusvalinnat : public TallentavaMaaritysWidget
{
    Q_OBJECT

public:
    Perusvalinnat();
    ~Perusvalinnat() override;

public:
    QString ohjesivu() override { return "maaritykset/perusvalinnat";}

    bool nollaa() override;
    bool onkoMuokattu() override;

public slots:
    void vaihdaLogo();
    void poistaLogo();

    bool tallenna() override;
    void alvilaajuudesta();

    void naytaLogo();

private:
    void avaaHakemisto();
    void vaihdaArkistoHakemisto();

private:
    void kokoSaapuu(QVariant* data);

private:
    Ui::Perusvalinnat *ui;
    QImage uusilogo;
    bool poistalogo = false;
};

#endif // PERUSVALINNAT_H
