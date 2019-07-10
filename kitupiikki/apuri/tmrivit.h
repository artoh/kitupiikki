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
#ifndef TMRIVIT_H
#define TMRIVIT_H

#include <QAbstractTableModel>

#include "db/tili.h"

namespace {

    struct Rivi {
        Tili tili;
        qlonglong maara;
        QString selite;
    };
}


class TmRivit : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TmRivit(QObject *parent = nullptr);

    enum Sarakkeet {
        TILI, EUROA
    };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


    void clear();

    void setTili(int rivi, Tili tili);
    Tili tili(int rivi) const;

    void setMaara(int rivi,qlonglong senttia);
    qlonglong maara(int rivi) const;

    void setSelite(int rivi, const QString& selite);
    QString selite(int rivi);

    int lisaaRivi();

private:
    QList<Rivi> rivit_;
};

#endif // TMRIVIT_H
