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
#ifndef KOHDENNUSMAARITYS_H
#define KOHDENNUSMAARITYS_H

#include "../maaritys/maarityswidget.h"

// Käytetään samaa käyttöliittymäpohjaa kuin kohdennusmäärityksissä
namespace Ui {
    class KiertoMaaritys;
}

class KiertoMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    KiertoMaaritys(QWidget *parent = nullptr);
    ~KiertoMaaritys() override;

    bool nollaa() override;
    bool naytetaankoTallennus() override { return false;}

private:
    void uusi();
    void muokkaa();
    void poista();

    void ohjeMuokattu();
    void paivitaNapit(const QModelIndex& index);

    Ui::KiertoMaaritys *ui;
};

#endif // KOHDENNUSMAARITYS_H
