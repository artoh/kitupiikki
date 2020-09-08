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
#ifndef TILAUSWIZARD_H
#define TILAUSWIZARD_H

#include <QWizard>
#include <QVariantMap>

class PlanModel;
class TilausValintaSivu;

namespace Ui {
    class TilausYhteys;
    class TilausVahvistus;
}

class TilausWizard : public QWizard
{
    Q_OBJECT
public:
    TilausWizard();
    int nextId() const override;

    enum { VALINTA, YHTEYSTIEDOT, VAHVISTUS};

    void nayta();

    QString yhteenveto();

protected:
    void dataSaapuu();
    void tilattu();

protected:
    PlanModel* planModel_;
    QVariantMap current_;

    TilausValintaSivu *valintaSivu_;

    class TilausYhteysSivu : public QWizardPage {
    public:
        TilausYhteysSivu();
    protected:
        Ui::TilausYhteys *ui;
    };

    class TilausVahvistusSivu : public QWizardPage {
    public:
        TilausVahvistusSivu(TilausWizard *velho);
        void initializePage() override;
    protected:
        Ui::TilausVahvistus *ui;
        TilausWizard *velho_;
    };
};

#endif // TILAUSWIZARD_H
