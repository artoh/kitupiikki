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
#ifndef KIERTOSELAUSMODEL_H
#define KIERTOSELAUSMODEL_H

#include <QAbstractTableModel>

class KiertoSelausModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit KiertoSelausModel(QObject *parent = nullptr);

    enum {
        TILA,
        KIERTO,
        PVM,
        ERAPVM,
        SUMMA,
        KUMPPANI,
        OTSIKKO
    };

    enum {
        IdRooli = Qt::UserRole
    };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lataa();

protected:
    void saapuu(QVariant* data);

private:
    QVariantList lista_;
    bool kaikki_ = false;
};

#endif // KIERTOSELAUSMODEL_H
