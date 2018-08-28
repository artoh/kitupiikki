/*
   Copyright (C) 2018 Arto Hyvättinen

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

#ifndef LASKURYHMAMODEL_H
#define LASKURYHMAMODEL_H


#include "laskumodel.h"
#include <QAbstractTableModel>

struct Laskutettava
{
    QString nimi;
    QString osoite;
    QString sahkoposti;
    bool lahetetty = false;
};


/**
 * @brief Ryhmälaskulla laskutettavat asiakkaat
 */
class LaskuRyhmaModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    LaskuRyhmaModel(QObject *parent = nullptr);

    enum Sarake
    {
        VIITE, NIMI, SAHKOPOSTI
    };

    enum
    {
        ViiteRooli = Qt::UserRole + 1,
        NimiRooli = Qt::UserRole + 2,
        OsoiteRooli = Qt::UserRole + 3,
        SahkopostiRooli = Qt::UserRole + 4
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    void lisaa(const QString& nimi, const QString& osoite, const QString& sahkoposti);
    bool onkoNimella(const QString& nimi);
    void sahkopostiLahetetty(int indeksiin);

protected:
    QList<Laskutettava> ryhma_;
    qulonglong pohjaviite_ = 0;

};

#endif // LASKURYHMAMODEL_H
