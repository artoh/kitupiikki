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
#ifndef TILIOTEAPURI_H
#define TILIOTEAPURI_H

#include "apuriwidget.h"

class TilioteModel;

namespace Ui {
class TilioteApuri;
}

class KirjausWg;

class TilioteApuri : public ApuriWidget
{
    Q_OBJECT
public:
    TilioteApuri(QWidget *parent = nullptr, Tosite* tosite = nullptr);
    virtual ~TilioteApuri() override;


protected:
    bool teeTositteelle() override;
    void teeReset() override;

protected slots:
    void lisaaRivi();
    void lisaaTyhjaRivi();
    void riviValittu();
    void muokkaa();
    void poista();


protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    Ui::TilioteApuri *ui;
    TilioteModel *model_;
    KirjausWg *kwg_;
};

#endif // TILIOTEAPURI_H
