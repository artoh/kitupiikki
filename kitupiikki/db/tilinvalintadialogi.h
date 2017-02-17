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

#ifndef TILINVALINTADIALOGI_H
#define TILINVALINTADIALOGI_H

#include <QDialog>
#include <QSortFilterProxyModel>

#include "tili.h"
#include "tilimodel.h"

namespace Ui {
class TilinValintaDialogi;
}

class TilinValintaDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit TilinValintaDialogi(QWidget *parent = 0);
    ~TilinValintaDialogi();

    Tili valittu() const;

public slots:
    void suodata(const QString& alku);
    void suodataTyyppi(const QString& regexp);
    void suodataSuosikit(bool suodatetaanko);
    void asetaModel(TiliModel *model);

protected slots:
    void klikattu(const QModelIndex& index);


private:
    Ui::TilinValintaDialogi *ui;
    QSortFilterProxyModel* proxyNimi;
    QSortFilterProxyModel* proxyTyyppi;
    QSortFilterProxyModel* proxyTila;

    QString tyyppiSuodatin;

public:
    /**
     * @brief Näyttää dialogin, josta voi valita tilin
     * @param alku
     * @return
     */
    static Tili valitseTili(const QString& alku, const QString& tyyppiSuodatin = QString("[ABCD]"), TiliModel *model = 0);

};

#endif // TILINVALINTADIALOGI_H
