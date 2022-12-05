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
#include "model/euro.h"

/**
 * @brief The TiliMuuntoModelin sisäinen tietorakenne
 */
struct TilinMuunnos
{
    TilinMuunnos(int numero = 0, QString nimi = QString(), int muunnettu = 0, Euro euroSaldo = Euro::Zero);
    QString tiliStr() const;

    int alkuperainenTilinumero;
    QString tilinNimi;
    int muunnettuTilinumero;    
    Euro saldo;
};

/**
 * @brief The TiliMuuntoModel class
 */
class TiliMuuntoModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        ALKUPERAINEN,
        NIMI,
        UUSI,
        SALDO
    };

    TiliMuuntoModel(QObject *parent = nullptr);
    TiliMuuntoModel(const QList<QPair<int, QString>> &tilit);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    int tilinumeroIndeksilla(int indeksi) const;
    Euro saldoIndeksilla(int indeksi) const;

    /**
     * @brief Tilien muunnostaulukko (alkuperäinen, muunnettu)
     * @return
     */
    QMap<QString,int> muunnettu();

    void lisaa(int numero, const QString& nimi, Euro euroSaldo = Euro::Zero);

    bool naytaMuuntoDialogi(QWidget* parent = nullptr, bool avaus = false);

protected:
    QList<TilinMuunnos> data_;
    QMap<QString,int> muunteluLista_;

    bool saldollinen_ = false;


};

#endif // TILIMUUNTOMODEL_H
