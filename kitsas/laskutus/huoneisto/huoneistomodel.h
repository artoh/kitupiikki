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
#ifndef HUONEISTOMODEL_H
#define HUONEISTOMODEL_H

#include <QAbstractTableModel>
#include <QVariantList>

#include "model/euro.h"
#include "laskutus/viitenumero.h"

#include "db/kitsasinterface.h"


class HuoneistoModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum HuoneistoSarake { NIMI, ASIAKAS, LASKUT, MAKSUT, AVOIN };
    enum {
        IdRooli = Qt::UserRole,
        ViiteRooli = Qt::UserRole + 1
    };


    explicit HuoneistoModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void paivita();

protected:
    void lataa(QVariant* data);

private:
    class HuoneistoTieto {
    public:
        HuoneistoTieto();
        HuoneistoTieto(int id, const QString& nimi, int asiakas,
                       const Euro& laskutettu, const Euro& maksettu);

        int id() const;
        QString nimi() const;
        int asiakas() const;
        Euro laskutettu() const;
        Euro maksettu() const;
        ViiteNumero viite() const;

    private:
        int id_ = 0;
        QString nimi_;
        int asiakas_ = 0;
        Euro laskutettu_;
        Euro maksettu_;
    };

    QList<HuoneistoTieto> huoneistot_;
};

#endif // HUONEISTOMODEL_H
