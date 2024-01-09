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
#include "tilikausi.h"

/**
 * @brief Tilikaudet
 *
 * Model tilikausien selaamiseen ja muokkaamiseen.
 * Tilikausien lisäykset ja poistot tallentuvat suoraan tietokantaan
 *
 */
class TilikausiModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        KAUSI,
        TASE,
        LIIKEVAIHTO,
        TULOS,
        LIITEKOKO,
        ARKISTOITU,
        TILINPAATOS,
        LYHENNE
    };

    enum
    {
        AlkaaRooli = Qt::UserRole + 1,
        PaattyyRooli = Qt::UserRole + 2,
        HenkilostoRooli = Qt::UserRole + 3,
        LyhenneRooli = Qt::UserRole + 4
    };


    TilikausiModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


    QVariant data(const QModelIndex &index, int role) const;


    Tilikausi tilikausiPaivalle(const QDate &paiva) const;
    int indeksiPaivalle(const QDate &paiva) const;
    bool onkoTilikautta(const QDate &paiva) const;
    Tilikausi tilikausiIndeksilla(int indeksi) const;
    Tilikausi& viiteIndeksilla(int indeksi);

    QDate kirjanpitoAlkaa() const;
    QDate kirjanpitoLoppuu() const;

    void lataa(const QVariantList& lista);
    void tallenna(const Tilikausi& kausi, const QDate& vanhaAlku = QDate());
    void poista(const QDate& alkupvm);

    QString tositeTunnus(int tunniste, const QDate& pvm, const QString& sarja, bool samakausi = false, bool vertailu = false) const;

public slots:
    void paivita();
    void paivitaKausitunnukset();

protected slots:
    void lataaData(const QVariant* lista);

protected:
    QList<Tilikausi> kaudet_;
};

#endif // TILIKAUSIMODEL_H
