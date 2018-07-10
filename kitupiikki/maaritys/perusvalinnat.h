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

#include "maarityswidget.h"

namespace Ui {
class Perusvalinnat;
}

/**
 * @brief Määrityswidget perusvalinnoille (nimi, y-tunnus, logo)
 */
class Perusvalinnat : public MaaritysWidget
{
    Q_OBJECT

public:
    Perusvalinnat();
    ~Perusvalinnat();

public:
    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;

    QString ohjesivu() override { return "maaritykset/perusvalinnat";}

public slots:
    void vaihdaLogo();
    void ilmoitaMuokattu();
    void avaaHakemisto();

private:
    Ui::Perusvalinnat *ui;
    QImage uusilogo;
};

#endif // PERUSVALINNAT_H
