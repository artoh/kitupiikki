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
#ifndef TILAUSVALINTASIVU_H
#define TILAUSVALINTASIVU_H

#include <QWizardPage>

namespace Ui {
    class TilausValinta;
}

class PlanModel;

class TilausValintaSivu : public QWizardPage
{
Q_OBJECT
public:
    TilausValintaSivu(PlanModel *model);
    void alusta(int nykyinen, bool kuukausittain, double palautus, double vakiokoko, int lisapilvet);


    QVariant tilaus(int rooli) const;

    bool isComplete() const override;
    void initializePage() override;

protected slots:
    void paivita();
protected:
    Ui::TilausValinta *ui;
    int alkuperainenPlan_ = 0;
    bool alunperinPuolivuodet_ = 0;
    double palautus_ = 0;
    int alunperinLisapilvet_ = 0;
    double vakiokoko_ = 2.0;
};

#endif // TILAUSVALINTASIVU_H
