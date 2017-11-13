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

#ifndef TILIMODEL_H
#define TILIMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QList>

#include "db/tili.h"

/**
 * @brief Tilit
 *
 * Tilien tiedot
 *
 *
 */
class TiliModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum Sarake
    {
        NRONIMI, NUMERO, NIMI, TYYPPI
    };

    enum
    {
        IdRooli = Qt::UserRole + 1,
        NroRooli = Qt::UserRole + 2,
        NimiRooli = Qt::UserRole + 3,
        NroNimiRooli = Qt::UserRole + 4,
        OtsikkotasoRooli = Qt::UserRole + 5,
        TyyppiRooli = Qt::UserRole + 6,
        YsiRooli = Qt::UserRole + 7,
        TilaRooli = Qt::UserRole + 8
    };


    TiliModel(QSqlDatabase *tietokanta, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    void lisaaTili( Tili uusi);
    void poistaRivi( int riviIndeksi );

    Tili tiliIdlla(int id) const;
    Tili tiliIndeksilla(int i) const { return tilit_.value(i); }
    Tili tiliNumerolla(int numero) const;
    /**
     * @brief Palauttaa tilin, jolle kirjataan edellisiltä tilikausilta kertynyt yli/alijäämä
     * @return
     */
    Tili edellistenYlijaamaTili() const;
    /**
     * @brief Palauttaa ensimmäisen halutun tyyppisen tilin
     *
     * Käytetään esim. alv-velkaa BV varten
     *
     * @param tyyppikoodi
     * @return
     */
    Tili tiliTyyppikoodilla(QString tyyppikoodi) const;
    JsonKentta *jsonIndeksilla(int i);

    static QMap<QString,QString> tiliTyyppiTaulu() { return tilityypit__; }
    static void luoTyyppiTaulut();

    bool onkoMuokattu() const;

public slots:
    void lataa();
    void tallenna();

protected:
    QSqlDatabase *tietokanta_;

    QList<Tili> tilit_;
    static QMap<QString,QString> tilityypit__;
    QList<int> poistetutIdt_;

};

#endif // TILIMODEL_H
