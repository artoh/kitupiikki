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

#ifndef ERANVALINTAMODEL_H
#define ERANVALINTAMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "tili.h"




/**
 * @brief Yhden tase-erän tiedot EranValintaModel:issa
 *
 */
struct TaseEra
{
    /**
     * @brief TaseEra
     * @param id Haettavan erän id
     */
    TaseEra(int id = 0);

    /**
     * @brief Hakee tase-erän avaavaan tositteen tunnisteen
     * @return
     */
    QString tositteenTunniste();

    int eraId;
    QDate pvm;
    QString selite;
    int saldoSnt = 0;
};

/**
 * @brief Tase-erän valintamodel
 *
 * Tase-erät kuvaavat tase-erittelyn kohteita. Jos seurattavalle tilille tehdään vienti,
 * joka ei kuulu aikaisempaan tase-erään, muodostuu uusi tase-erä. Tilin kirjaus voidaan
 * kohdistaa tälle erälle, ellei tase-erä ole jo tasan (debet=kredit)
 *
 */
class EranValintaModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum
    {
        EraIdRooli = Qt::UserRole,
        PvmRooli = Qt::UserRole +1,
        SeliteRooli = Qt::UserRole +2,
        SaldoRooli = Qt::UserRole + 3,
        TositteenTunnisteRooli = Qt::UserRole + 4
    };


    EranValintaModel();

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    /**
     * @brief Lataa erät
     * @param tili Tili, jonka erät ladataan
     * @param kaikki tosi jos ladataan myös tasan menneet erät
     * @param paivalle Päivämäärä, jolta erät lasketaan
     */
    void lataa(Tili tili, bool kaikki = false, QDate paivalle = QDate());

private:
    QList<TaseEra> erat_;
};

#endif // ERANVALINTAMODEL_H
