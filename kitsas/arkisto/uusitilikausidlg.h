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
#ifndef UUSITILIKAUSIDLG_H
#define UUSITILIKAUSIDLG_H

#include <QDialog>

namespace Ui {
    class UusiTilikausiDlg;
}


class UusiTilikausiDlg : public QDialog
{
    Q_OBJECT
public:
    UusiTilikausiDlg(QWidget* parent = nullptr);

    void accept() override;

private:
    void alkuun();
    void loppuun();

    Ui::UusiTilikausiDlg* ui;
};

#endif // UUSITILIKAUSIDLG_H
