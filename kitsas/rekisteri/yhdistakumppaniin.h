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
#ifndef YHDISTAKUMPPANIIN_H
#define YHDISTAKUMPPANIIN_H

#include <QDialog>

#include "yhdistamisproxymodel.h"
#include "laskutus/asiakkaatmodel.h"

namespace Ui {
class YhdistaKumppaniin;
}

class YhdistaKumppaniin : public QDialog
{
    Q_OBJECT

public:
    YhdistaKumppaniin(AsiakkaatModel * model, int id, const QString& nimi, QWidget *parent = nullptr);
    ~YhdistaKumppaniin();

    void accept() override;

protected:
    void suodata(bool onko);
    void valmis();

private:
    Ui::YhdistaKumppaniin *ui;
    YhdistamisProxyModel* proxy;

    int id_;
    QString nimi_;
};

#endif // YHDISTAKUMPPANIIN_H
