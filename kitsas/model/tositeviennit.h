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
#ifndef TOSITEVIENNIT_H
#define TOSITEVIENNIT_H

#include <QAbstractTableModel>

#include "tositevienti.h"

class TositeViennit : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TositeViennit(QObject *parent = nullptr);

    enum VientiSarake
    {
        PVM, TILI, DEBET, KREDIT, KOHDENNUS, ALV, SELITE
    };

    enum
    {
        IdRooli = Qt::UserRole + 1,
        PvmRooli = Qt::UserRole + 2,
        TiliNumeroRooli = Qt::UserRole + 3,
        DebetRooli = Qt::UserRole + 4,
        KreditRooli = Qt::UserRole + 5,
        AlvKoodiRooli = Qt::UserRole + 6,
        AlvProsenttiRooli = Qt::UserRole + 7,
        KohdennusRooli = Qt::UserRole + 8,
        SeliteRooli = Qt::UserRole + 9,
        EraIdRooli = Qt::UserRole + 13,
        PoistoKkRooli = Qt::UserRole + 14,
        TaseErittelyssaRooli = Qt::UserRole + 15,
        TagiIdListaRooli = Qt::UserRole + 22,
        EraMapRooli = Qt::UserRole + 23
    };

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

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    QModelIndex lisaaVienti(int indeksi);

    TositeVienti vienti(int indeksi) const;
    void lisaa(const TositeVienti& vienti);

    void asetaViennit(QVariantList viennit);
    void tyhjenna();
    void pohjaksi(const QDate& pvm, const QString& vanhaOtsikko, const QString& uusiOtsikko);

    QVariant viennit() const { return viennit_;}
    QVariantList vientilLista() const { return viennit_; }


    QVariant tallennettavat() const;

    void asetaMuokattavissa(bool muokattavissa);
    bool muokattavissa() const { return muokattavissa_;}

    QString alvTarkastus() const;

private:
    QVariantList viennit_;
    bool muokattavissa_ = true;

};

#endif // TOSITEVIENNIT_H
