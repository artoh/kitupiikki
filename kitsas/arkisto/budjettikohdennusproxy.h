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
#ifndef BUDJETTIKOHDENNUSPROXY_H
#define BUDJETTIKOHDENNUSPROXY_H

#include <QSortFilterProxyModel>
#include <QDate>

/**
 * @brief BudjettiDlg:n käyttöön proxy kohdennusten suodattamiseen
 *
 * Suodattaa ne kustannuspaikat ja projektit, jotka käytössä
 * tällä tilikaudella
 *
 */
class BudjettiKohdennusProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    BudjettiKohdennusProxy(QObject* parent = nullptr);

    void asetaKausi(const QDate& alkaa, const QDate &paattyy);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    QDate alkaa_;
    QDate paattyy_;
};

#endif // BUDJETTIKOHDENNUSPROXY_H
