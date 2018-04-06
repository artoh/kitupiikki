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

#ifndef TILIKAUSIMODEL_H
#define TILIKAUSIMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>

#include "tilikausi.h"

/**
 * @brief Tilikaudet
 *
 * Model tilikausien selaamiseen ja muokkaamiseen.
 * Tilikausien lisäykset ja poistot tallentuvat suoraan tietokantaan,
 * json-kenttien muokkaamisen jälkeen on vielä erikseen valittava tallenna()
 *
 */
class TilikausiModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        KAUSI,
        LIIKEVAIHTO,
        TASE,
        TULOS,
        ARKISTOITU,
        TILINPAATOS,
        LYHENNE
    };

    enum
    {
        AlkaaRooli = Qt::UserRole + 1,
        PaattyyRooli = Qt::UserRole + 2,
        HenkilostoRooli = Qt::UserRole + 3
    };


    TilikausiModel(QSqlDatabase *tietokanta, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


    QVariant data(const QModelIndex &index, int role) const;

    void lisaaTilikausi( Tilikausi tilikausi);

    /**
     * @brief Muuttaa viimeisen tilikauden päätöspäivän tai poistaa sen
     * @param paattyy Uusi päättymispäivä tai QDate() jos poistetaan
     */
    void muokkaaViimeinenTilikausi(const QDate& paattyy);

    Tilikausi tilikausiPaivalle(const QDate &paiva) const;
    int indeksiPaivalle(const QDate &paiva) const;
    Tilikausi tilikausiIndeksilla(int indeksi) const;

    /**
     * @brief Tilikauden json-kentät
     * @param indeksi
     * @return Osoitin Json-kenttiin
     */
    JsonKentta* json(int indeksi);
    JsonKentta *json(Tilikausi tilikausi);
    JsonKentta* json(const QDate &paiva);

    QDate kirjanpitoAlkaa() const;
    QDate kirjanpitoLoppuu() const;

public slots:
    void lataa();

    /**
     * @brief tallenna Tallentaa muutetut json-kentät
     */
    void tallennaJSON();

    void paivitaKausitunnukset();

protected:
    QSqlDatabase *tietokanta_;
    QList<Tilikausi> kaudet_;
};

#endif // TILIKAUSIMODEL_H
