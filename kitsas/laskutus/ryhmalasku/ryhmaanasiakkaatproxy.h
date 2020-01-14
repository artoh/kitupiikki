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
#ifndef RYHMAANASIAKKAATPROXY_H
#define RYHMAANASIAKKAATPROXY_H

#include <QSortFilterProxyModel>

class LaskutettavatModel;

class RyhmaanAsiakkaatProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    RyhmaanAsiakkaatProxy(QObject *parent = nullptr);
    void asetaLaskutettavatModel( LaskutettavatModel *model);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    LaskutettavatModel *laskutettavat_ = nullptr;
};

#endif // RYHMAANASIAKKAATPROXY_H
