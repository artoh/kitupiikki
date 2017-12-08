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

#ifndef LASKUNMAKSUDIALOGI_H
#define LASKUNMAKSUDIALOGI_H

#include <QDialog>

namespace Ui {
class LaskunMaksuDialogi;
}

class LaskunMaksuDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit LaskunMaksuDialogi(QWidget *parent = 0);
    ~LaskunMaksuDialogi();

private:
    Ui::LaskunMaksuDialogi *ui;
};

#endif // LASKUNMAKSUDIALOGI_H
