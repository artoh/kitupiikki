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

#ifndef LASKULISTAMODEL_H
#define LASKULISTAMODEL_H

#include <QSqlQueryModel>
#include <QDate>

/**
 * @brief Laskujen listaus
 *
 * Toteutettu sql-kyselyllä. Aktioitava paivita-funktiolla.
 */
class LaskulistaModel : public QSqlQueryModel
{
Q_OBJECT

public:
    LaskulistaModel(QObject *parent = 0);

    enum Laskuvalinta { KAIKKI, AVOIMET, ERAANTYNEET };
    enum LaskuSarake { NUMERO, PVM, ERAPVM, SUMMA, MAKSAMATTA, TOSITE, ASIAKAS};

    QVariant data(const QModelIndex &item, int role) const;

public slots:
    void paivita(int valinta = KAIKKI, QDate mista=QDate(), QDate mihin=QDate());
};

#endif // LASKULISTAMODEL_H
