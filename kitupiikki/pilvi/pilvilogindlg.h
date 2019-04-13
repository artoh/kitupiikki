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
#ifndef PILVILOGINDLG_H
#define PILVILOGINDLG_H

#include <QDialog>

namespace Ui {
class PilviLoginDlg;
}

class PilviLoginDlg : public QDialog
{
    Q_OBJECT

public:
    explicit PilviLoginDlg(QWidget *parent = nullptr);
    virtual ~PilviLoginDlg() override;

public slots:
    virtual void accept() override;

private slots:
    void validoi();
    void lahetetty();

private:
    Ui::PilviLoginDlg *ui;
};

#endif // PILVILOGINDLG_H
