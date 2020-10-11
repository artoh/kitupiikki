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
#ifndef VAKIOVIITEDLG_H
#define VAKIOVIITEDLG_H

#include <QDialog>

class VakioViiteModel;

namespace Ui {
class VakioViiteDlg;
}

class VakioViiteDlg : public QDialog
{
    Q_OBJECT

public:
    explicit VakioViiteDlg(VakioViiteModel* model, QWidget *parent = nullptr);
    ~VakioViiteDlg() override;

    void muokkaa(const QVariantMap& map);
    void uusi();

    void tarkasta();

    void accept() override;

private:
    void tallennettu();

    VakioViiteModel* model_;

    Ui::VakioViiteDlg *ui;
};

#endif // VAKIOVIITEDLG_H
