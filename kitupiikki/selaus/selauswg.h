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

#ifndef SELAUSWG_H
#define SELAUSWG_H

#include <QWidget>

#include "ui_selauswg.h"

class SelausModel;
class QSortFilterProxyModel;

class SelausWg : public QWidget
{
    Q_OBJECT

public:
    SelausWg();
    ~SelausWg();

public slots:
    void alusta();
    void paivita();
    void suodata();
    void paivitaSummat();
    void naytaTositeRivilta(QModelIndex index);

signals:
    void tositeValittu(int id);

private:
    Ui::SelausWg *ui;
    SelausModel *model;
    QSortFilterProxyModel *proxyModel;
};

#endif // SELAUSWG_H
