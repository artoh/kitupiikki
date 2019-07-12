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
        int tilinumero;
        qlonglong maara = 0;
        qlonglong netto = 0;
        QString selite;
        int verokoodi = 0;
        double veroprosentti = 0.0;
        bool eivahennysta = false;
        int kohdennus = 0;
        QVariantList merkkaukset;
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

    void setTili(int rivi, int tili);
    Tili tili(int rivi) const;

    void setMaara(int rivi,qlonglong senttia);
    qlonglong maara(int rivi) const;

    void setNetto(int rivi, qlonglong senttia);
    qlonglong netto(int rivi) const;

    void setAlvKoodi(int rivi, int koodi);
    int alvkoodi(int rivi) const;

    void setAlvProsentti(int rivi, double prosentti);
    double alvProsentti(int rivi) const;

    void setSelite(int rivi, const QString& selite);
    QString selite(int rivi) const;

    void setEiVahennysta(int rivi, bool eivahennysta);
    bool eiVahennysta(int rivi) const;

    void setKohdennus(int rivi, int kohdennus);
    int kohdennus(int rivi) const;

    void setMerkkaukset(int rivi, QVariantList);
    QVariantList merkkaukset(int rivi) const;

    int lisaaRivi();
    void poistaRivi(int rivi);

private:
    QList<Rivi> rivit_;
};

#endif // TMRIVIT_H
