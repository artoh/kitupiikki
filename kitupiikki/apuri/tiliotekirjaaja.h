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
#ifndef TILIOTEKIRJAAJA_H
#define TILIOTEKIRJAAJA_H

#include <QDialog>

namespace Ui {
class TilioteKirjaaja;
}

class TilioteKirjaaja : public QDialog
{
    Q_OBJECT    
public:
    enum AlaTab { MAKSU, TULOMENO, SIIRTO, PIILOSSA };

    explicit TilioteKirjaaja(QWidget *parent = nullptr);
    ~TilioteKirjaaja();

    void asetaPvm(const QDate& pvm);

private slots:
    void alaTabMuuttui(int tab);
    void euroMuuttuu();

private:
    Ui::TilioteKirjaaja *ui;

    int menoa_ = false;
};

#endif // TILIOTEKIRJAAJA_H
