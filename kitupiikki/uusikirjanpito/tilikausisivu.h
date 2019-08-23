/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef TILIKAUSISIVU_H
#define TILIKAUSISIVU_H

#include <QWizardPage>


class UusiVelho;

namespace Ui {
    class TilikausiSivu;
}

/**
 * @brief Uuden kirjanpidon velhon sivu, jossa määritellään nykyinen ja edellinen tilikausi
 */
class TilikausiSivu : public QWizardPage
{
    Q_OBJECT

protected:
    Ui::TilikausiSivu *ui;

public:
    TilikausiSivu(UusiVelho *wizard);
    ~TilikausiSivu() override;

    bool isComplete() const override;
    bool validatePage() override;

public slots:
    void alkuPaivaMuuttui(const QDate& date);
    void loppuPaivaMuuttui();

private:
    UusiVelho *velho;

};

#endif // TILIKAUSISIVU_H
