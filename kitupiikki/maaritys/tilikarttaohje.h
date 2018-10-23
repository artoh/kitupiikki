/*
   Copyright (C) 2018 Arto Hyvättinen

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

#ifndef TILIKARTTAOHJE_H
#define TILIKARTTAOHJE_H

#include "maarityswidget.h"

namespace Ui {
class TilikarttaOhje;
}

/**
 * @brief Näyttää tilikartan ohjeet
 */
class TilikarttaOhje : public MaaritysWidget
{
    Q_OBJECT
public:
    TilikarttaOhje();    
    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;
    bool naytetaankoVienti()  override { return true; }

public slots:
    void devtool();

private:
    Ui::TilikarttaOhje *ui;
    bool muokattu = false;
};

#endif // TILIKARTTAOHJE_H
