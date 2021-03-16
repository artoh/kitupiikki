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
#ifndef TOSITERIVIT_H
#define TOSITERIVIT_H

#include <QAbstractTableModel>
#include <QVariantList>
#include <QDate>

#include "tosite.h"
#include "tositerivi.h"

#include "laskutus/yksikkomodel.h"

class TositeRivit : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum LaskuSarake
    {
        NIMIKE, MAARA, YKSIKKO, AHINTA, ALE, TILI, ALV, KOHDENNUS, BRUTTOSUMMA
    };

    enum
    {
        TiliNumeroRooli = Qt::UserRole + 3,
        AlvKoodiRooli = Qt::UserRole + 5,
        AlvProsenttiRooli = Qt::UserRole + 6,
        UNkoodiRooli = Qt::UserRole + 256
    };


    TositeRivit(QObject *parent = nullptr, const QVariantList& data = QVariantList());
    void lataa(const QVariantList& data);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariantList rivit() const;

    Euro yhteensa() const;
    QVariantList viennit(const Tosite& tosite) const;

    bool onkoTyhja() const;

    TositeRivi rivi(int indeksi) const;
    void asetaRivi(int indeksi, const TositeRivi& rivi);

public slots:
    void lisaaRivi(QVariantMap rivi = QVariantMap());
    void poistaRivi(int rivi);
    void asetaEnnakkolasku(bool ennakkoa);


private:
    QList<TositeRivi> rivit_;

    YksikkoModel yksikkoModel_;

    bool ennakkolasku_ = false;
};

#endif // TOSITERIVIT_H
