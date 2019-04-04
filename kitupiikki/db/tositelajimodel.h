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

#ifndef TOSITELAJIMODEL_H
#define TOSITELAJIMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>

#include "tositelaji.h"


/**
 * @brief Tositelajien model
 */

class TositelajiModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        TUNNUS, NIMI, VASTATILI
    };

    enum KirjausTyyppi
    {
        KAIKKIKIRJAUKSET,
        OSTOLASKUT,
        MYYNTILASKUT,
        TILIOTE
    };

    enum
    {
        IdRooli = Qt::UserRole,
        TunnusRooli = Qt::UserRole + 1,
        NimiRooli = Qt::UserRole + 2,
        VastatiliNroRooli = Qt::UserRole + 3,
        TositeMaaraRooli = Qt::UserRole +4,
        KirjausTyyppiRooli = Qt::UserRole + 5,
        OletustiliRooli = Qt::UserRole + 6,
        JsonRooli = Qt::UserRole + 7

    };

    TositelajiModel(QSqlDatabase *tietokanta, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);
    bool onkoMuokattu() const;

    void poistaRivi(int riviIndeksi);

    Tositelaji tositelajiVanha(int id) const;
    Tositelaji *tositeLaji(int id) const;

    QModelIndex lisaaRivi();

    void lataa(const QVariantList& lista);

public slots:
    void lataa();
    bool tallenna();

protected:
    QList<Tositelaji> lajitVanha_;

    QHash<int, Tositelaji*> idHash_;

    QSqlDatabase *tietokanta_;
    QList<int> poistetutIdt_;
};

#endif // TOSITELAJIMODEL_H
