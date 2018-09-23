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

#ifndef TILIMUUNTOMODEL_H
#define TILIMUUNTOMODEL_H

#include <QAbstractTableModel>

/**
 * @brief The TiliMuuntoModelin sisäinen tietorakenne
 */
struct TilinMuunnos
{
    TilinMuunnos(int numero = 0, QString nimi = QString());

    int alkuperainenTilinumero;
    QString tilinNimi;
    int muunnettuTilinumero;
};

/**
 * @brief The TiliMuuntoModel class
 */
class TiliMuuntoModel : public QAbstractTableModel
{
public:
    enum Sarake
    {
        ALKUPERAINEN,
        NIMI,
        UUSI
    };

    TiliMuuntoModel(const QList<QPair<int, QString>> &tilit);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    /**
     * @brief Tilien muunnostaulukko (alkuperäinen, muunnettu)
     * @return
     */
    QMap<QString,int> muunnettu();

protected:
    QList<TilinMuunnos> data_;


};

#endif // TILIMUUNTOMODEL_H
