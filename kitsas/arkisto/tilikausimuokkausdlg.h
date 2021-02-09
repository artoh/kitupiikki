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
#ifndef TILIKAUSIMUOKKAUSDLG_H
#define TILIKAUSIMUOKKAUSDLG_H

#include <QDialog>

#include "db/tilikausi.h"

class QPushButton;

namespace Ui {
class TilikausiMuokkausDlg;
}

class TilikausiMuokkausDlg : public QDialog
{
    Q_OBJECT

public:
    explicit TilikausiMuokkausDlg(QWidget *parent = nullptr);
    ~TilikausiMuokkausDlg();

    void muokkaa(Tilikausi kausi);

    void accept() override;

protected:
    void tarkastaKausi();
    void puralukko();
    void muutaAvaus();
    void poista();
    void tilinavausId(QVariant* data);

private:
    Ui::TilikausiMuokkausDlg *ui;
    QPushButton* pbtn;

    Tilikausi kausi_;
    QDate alkuperainenAlkupaiva_;
    int tilinavausId_ = 0;

};

#endif // TILIKAUSIMUOKKAUSDLG_H
