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
#ifndef HUONEISTODIALOG_H
#define HUONEISTODIALOG_H

#include "huoneisto.h"

#include <QDialog>

class QSortFilterProxyModel;

namespace Ui {
class HuoneistoDialog;
}

class HuoneistoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HuoneistoDialog(QWidget *parent = nullptr);
    ~HuoneistoDialog();

    void lataa(int id);
    void accept() override;

protected:
    void naytolle();
    void tallennettu();
    void lisaaTuote();
    void valintaMuuttui();
    void poistaRivi();

private:
    Ui::HuoneistoDialog *ui;

    Huoneisto huoneisto_;
    QSortFilterProxyModel* proxy_;
};

#endif // HUONEISTODIALOG_H
