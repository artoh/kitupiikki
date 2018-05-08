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

#ifndef TUONTIMAARITYSWIDGET_H
#define TUONTIMAARITYSWIDGET_H

#include "maarityswidget.h"
#include "ui_tuontimaaritys.h"

/**
 * @brief Tuonnin määritykset
 */
class TuontiMaaritysWidget : public MaaritysWidget
{
    Q_OBJECT
public:
    TuontiMaaritysWidget();
    ~TuontiMaaritysWidget();

    bool nollaa();
    bool tallenna();
    bool onkoMuokattu();

    QString ohjesivu() override { return "kirjaus/tuonti"; }

public slots:
    void ilmoitaMuokattu();

private:
    Ui::TuontiMaaritys *ui;

};

#endif // TUONTIMAARITYSWIDGET_H
