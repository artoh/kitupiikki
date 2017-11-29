/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef LASKUMODEL_H
#define LASKUMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include <QList>

#include "db/tili.h"
#include "db/kohdennus.h"


struct LaskuRivi
{
    LaskuRivi();
    double yhteensaSnt() const;

    QString nimike;
    double maara = 1.00;
    QString yksikko;
    double ahintaSnt = 0.00;
    int alvKoodi;
    int alvProsentti = 0.00;
    Tili myyntiTili;
    Kohdennus kohdennus;
};

class LaskuModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    LaskuModel(QObject *parent = 0);

    enum LaskuSarake
    {
        NIMIKE, MAARA, YKSIKKO, AHINTA, ALV, TILI, KOHDENNUS, BRUTTOSUMMA
    };

    enum
    {
        AlvKoodiRooli = Qt::UserRole + 5,
        AlvProsenttiRooli = Qt::UserRole + 6,
        NettoRooli = Qt::UserRole + 7,
        VeroRooli = Qt::UserRole + 8
    };


    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    int laskunSumma() const;


    QDate erapaiva() const { return erapaiva_; }
    QString lisatieto() const { return lisatieto_;}
    QString osoite() const { return osoite_; }

    void asetaErapaiva(const QDate & paiva) { erapaiva_ = paiva; }
    void asetaLisatieto(const QString& tieto) { lisatieto_ = tieto; }
    void asetaOsoite(const QString& osoite) { osoite_ = osoite; }

public slots:
    QModelIndex lisaaRivi(LaskuRivi rivi = LaskuRivi());

signals:
    void summaMuuttunut(int summaSnt);

private:
    QList<LaskuRivi> rivit_;
    QDate erapaiva_;
    QString lisatieto_;
    QString osoite_;

    void paivitaSumma(int rivi);
};

#endif // LASKUMODEL_H
