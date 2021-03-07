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
#ifndef TILIOTEVIENNIT_H
#define TILIOTEVIENNIT_H

#include <QAbstractTableModel>

#include "tiliotekirjausrivi.h"

class KitsasInterface;

class TilioteViennit : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum { TILI, EURO };

    explicit TilioteViennit(KitsasInterface* interface, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void tyhjenna();
    void lisaaVienti(const TositeVienti& vienti);
    void poistaVienti(int indeksi);
    void asetaVienti(int indeksi, const TositeVienti& vienti);
    TositeVienti vienti(int indeksi) const;
    QList<TositeVienti> viennit() const;

    qlonglong summa() const;

private:
    QList<TositeVienti> viennit_;
    KitsasInterface* kitsasInterface_;


};

#endif // TILIOTEVIENNIT_H
