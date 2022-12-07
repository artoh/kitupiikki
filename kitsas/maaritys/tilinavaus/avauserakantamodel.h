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
#ifndef AVAUSERAKANTAMODEL_H
#define AVAUSERAKANTAMODEL_H

#include <QAbstractTableModel>

#include "tilinavausmodel.h"

class AvausEraKantaModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum { KUMPPANI, NIMI, SALDO, POISTOAIKA };

    AvausEraKantaModel(QObject *parent = nullptr);
    AvausEraKantaModel(QList<AvausEra> erat = QList<AvausEra>(),
                                QObject *parent = nullptr);

    // Basic functionality:
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual QList<AvausEra> erat() const;
    virtual Euro summa() const;
    virtual void lataa(QList<AvausEra> erat);

protected:
    QList<AvausEra> erat_;

};

#endif // AVAUSERAKANTAMODEL_H
