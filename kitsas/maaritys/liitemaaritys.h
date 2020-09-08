/*
   Copyright (C) 2019 Arto Hyvättinen

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
#ifndef LIITEMAARITYS_H
#define LIITEMAARITYS_H

#include "maarityswidget.h"

namespace Ui {
    class LiiteMaaritys;
}

class LiiteMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    LiiteMaaritys();
    ~LiiteMaaritys() override;

    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;
    bool naytetaankoTallennus() override { return true; }

private:
    void ilmoitaMuokattu();

private:
    Ui::LiiteMaaritys *ui;
};

#endif // LIITEMAARITYS_H
