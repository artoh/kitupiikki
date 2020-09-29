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
#ifndef PLANMODEL_H
#define PLANMODEL_H

#include <QAbstractTableModel>

class PlanModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    PlanModel( QObject *parent = nullptr);

    enum { NIMI, HINTA};

    enum { PlanRooli = Qt::UserRole,
           HintaRooli = Qt::UserRole +1,
           NimiRooli = Qt::UserRole + 2,
           PilviaRooli = Qt::UserRole + 3,
           LisaPilviHinta  = Qt::UserRole + 4,
           InfoRooli = Qt::UserRole + 5
         };

    enum { TILITOIMISTOPLAN = 50 };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int rowForPlan(int plan) const;

    void alusta(const QVariantList& plans, bool kuukausittain=false);

public slots:
    void naytaPuolivuosittain(bool puolivuosittain);

private:
    QVariantList plans_;
    bool puolivuosittain_;
    int pilvia_;
};

#endif // PLANMODEL_H
