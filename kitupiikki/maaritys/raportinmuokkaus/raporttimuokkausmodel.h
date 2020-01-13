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
#ifndef RAPORTTIMUOKKAUSMODEL_H
#define RAPORTTIMUOKKAUSMODEL_H

#include <QAbstractTableModel>

#include <QVariantList>

class RaporttiMuokkausModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RaporttiMuokkausModel(QObject *parent = nullptr);

    enum { TEKSTI, TYYPPI, KAAVA };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void lataa(const QVariantList& lista);

private:
    QVariantList rivit_;
};

#endif // RAPORTTIMUOKKAUSMODEL_H
