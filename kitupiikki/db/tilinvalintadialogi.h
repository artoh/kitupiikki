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

#ifndef TILINVALINTADIALOGI_H
#define TILINVALINTADIALOGI_H

#include <QDialog>

#include "tili.h"

namespace Ui {
class TilinValintaDialogi;
}

class TilinValintaDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit TilinValintaDialogi(QWidget *parent = 0);
    ~TilinValintaDialogi();

private:
    Ui::TilinValintaDialogi *ui;

public:
    static Tili valitseTili(const QString& alku);
};

#endif // TILINVALINTADIALOGI_H
