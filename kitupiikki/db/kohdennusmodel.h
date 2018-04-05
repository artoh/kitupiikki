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

#ifndef KOHDENNUSMODEL_H
#define KOHDENNUSMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include <QList>
#include <QSqlDatabase>

#include "kohdennus.h"


/**
 * @brief Kohdennusten luettelo
 *
 *
 */
class KohdennusModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum Sarake
    {
        NIMI, ALKAA, PAATTYY
    };

    enum
    {
        IdRooli = Qt::UserRole + 1,
        TyyppiRooli = Qt::UserRole + 2,
        NimiRooli = Qt::UserRole + 3,
        AlkaaRooli = Qt::UserRole + 4,
        PaattyyRooli = Qt::UserRole + 5,
        VientejaRooli = Qt::UserRole + 6
    };

    KohdennusModel(QSqlDatabase *tietokanta, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QString nimi(int id) const;
    Kohdennus kohdennus(const int id) const;
    Kohdennus kohdennus(const QString& nimi) const;
    QList<Kohdennus> kohdennukset() const;

    /**
     * @brief Onko määritelty kustannuspaikkoja tai projekteja
     * @return
     */
    bool kohdennuksia() const;
    /**
     * @brief Onko määritelty merkkauksia
     * @return
     */
    bool merkkauksia() const;

    /**
     * @brief Poistaa kohdennuksen
     * @param riviIndeksi Indeksi (index.row()) poistettavaan
     */
    void poistaRivi(int riviIndeksi);

    bool onkoMuokattu() const;

public slots:
    void lataa();
    void lisaaUusi(Kohdennus uusi);
    void tallenna();


protected:
    QSqlDatabase *tietokanta_;
    QList<Kohdennus> kohdennukset_;
    QList<int> poistetutIdt_;


};

#endif // KOHDENNUSMODEL_H
