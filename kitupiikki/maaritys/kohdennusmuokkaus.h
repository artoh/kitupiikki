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

#ifndef KOHDENNUSMUOKKAUS_H
#define KOHDENNUSMUOKKAUS_H

#include <QSortFilterProxyModel>

#include "maarityswidget.h"
#include "ui_kohdennukset.h"
#include "kohdennusmuokkaus.h"

#include "db/kohdennusmodel.h"

/**
 * @brief Kohdennusten muokkaussivu
 */
class KohdennusMuokkaus : public MaaritysWidget
{
    Q_OBJECT
public:
    KohdennusMuokkaus(QWidget *parent = 0);
    ~KohdennusMuokkaus();

    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;

public slots:
    void uusi();
    void muokkaa();
    void poista();

    QString ohjesivu() override { return "maaritykset/kohdennukset"; }

    /**
     * @brief Kun rivi on valittu, merkitsee mitä nappia voi painaa (Muokkaa, Poista)
     * @param index
     */
    void riviValittu(const QModelIndex& index);

protected:
    Ui::Kohdennukset *ui;
    KohdennusModel *model;
    QSortFilterProxyModel *proxy;

};

#endif // KOHDENNUSMUOKKAUS_H
