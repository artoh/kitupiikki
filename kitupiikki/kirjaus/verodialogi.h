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

#ifndef VERODIALOGI_H
#define VERODIALOGI_H

#include <QDialog>

struct VeroDialogiValinta
{
    int verokoodi;
    int veroprosentti;
};


namespace Ui {
class VeroDialogi;
}

class VeroDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit VeroDialogi(QWidget *parent = 0);
    ~VeroDialogi();

public slots:
    void lajimuuttuu();

private:
    Ui::VeroDialogi *ui;

public:
    static VeroDialogiValinta veroDlg(int koodi, int prosentti, bool tyyppilukko = false);
};

#endif // VERODIALOGI_H
