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

#ifndef TILIKAUSIMAARITYKSET_H
#define TILIKAUSIMAARITYKSET_H

#include "maarityswidget.h"
#include "ui_tilikausimaaritykset.h"

class TilikausiMaaritykset : public MaaritysWidget
{
    Q_OBJECT
public:
    TilikausiMaaritykset();
    ~TilikausiMaaritykset();

    bool nollaa();

public slots:
    static void uusiTilikausi();

private:
    Ui::TilikausiMaaritykset *ui;
};

#endif // TILIKAUSIMAARITYKSET_H
