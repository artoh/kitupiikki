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
#ifndef ASIAKASDLG_H
#define ASIAKASDLG_H

#include <QDialog>

class Asiakas;

namespace Ui {
class AsiakasDlg;
}

class AsiakasDlg : public QDialog
{
    Q_OBJECT

public:
    AsiakasDlg(QWidget *parent, Asiakas* asiakas);
    ~AsiakasDlg() override;

public slots:
    void accept() override;

private slots:
    void haeToimipaikka();

private:
    Ui::AsiakasDlg *ui;
    Asiakas* asiakas_;
};

#endif // ASIAKASDLG_H
