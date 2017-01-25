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

#ifndef SELAUSMODEL_H
#define SELAUSMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QDate>

#include "db/tili.h"



struct SelausRivi
{
    int tositeId;
    QDate pvm;
    Tili tili;
    QString selite;
    int debetSnt;
    int kreditSnt;
};



class SelausModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum SelausSarake
    {
        PVM, TILI, DEBET, KREDIT, SELITE
    };

    SelausModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    QStringList kaytetytTilit() const { return tileilla; }

public slots:
    void lataa(const QDate& alkaa, const QDate& loppuu);

protected:
    QList<SelausRivi> rivit;
    QStringList tileilla;

};

#endif // SELAUSMODEL_H
