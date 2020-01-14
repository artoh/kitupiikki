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
#ifndef TIEDOTSIVU_H
#define TIEDOTSIVU_H

#include "uusivelho.h"
#include <QWizardPage>

namespace Ui {
    class UusiTiedot;
}

class TiedotSivu : public QWizardPage {
    Q_OBJECT
public:
    TiedotSivu(UusiVelho *wizard);
    void initializePage() override;
    bool validatePage() override;

protected slots:
    void haeytunnarilla();
    void yTietoSaapuu();

protected:
    Ui::UusiTiedot *ui;
    UusiVelho *velho;
};

#endif // TIEDOTSIVU_H
