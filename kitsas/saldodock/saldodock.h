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
#ifndef SALDODOCK_H
#define SALDODOCK_H

#include <QDockWidget>

class SaldoModel;
class QTableView;
class QToolButton;
class QSortFilterProxyModel;

class SaldoDock : public QDockWidget
{
    Q_OBJECT

public:
    static SaldoDock* dock();

    enum { RAHAVARAT, SUOSIKIT, KAIKKI };
    void alusta();

private:
    SaldoDock();
    void paivita();

protected:
    void showEvent(QShowEvent* event) override;
    void setFilter(int index);

private:
    QTableView* view_;
    SaldoModel* model_;

    QToolButton *rahavarat_;
    QToolButton *suosikit_;
    QToolButton *kaikki_;

    QSortFilterProxyModel *proxy_;

    static SaldoDock* instanssi__;
};

#endif // SALDODOCK_H
