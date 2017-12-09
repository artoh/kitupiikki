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

#ifndef AVOIMETLASKUTMODEL_H
#define AVOIMETLASKUTMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QDate>

/**
 * @brief Laskunmaksudialogissa näytettävä avoin lasku
 */
struct AvoinLasku
{
    AvoinLasku() {}

    int viitenro;
    QDate pvm;
    QDate erapvm;
    int summaSnt;
    int avoinSnt;
    QString asiakas;
    int tosite;
    QByteArray json;
};

/**
 * @brief Laskulistan model
 *
 * Käytetään laskuluettelossa sekä laskudialogissa.
 *
 */
class LaskutModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    LaskutModel(QObject *parent = 0);


    enum Laskuvalinta { KAIKKI, AVOIMET, ERAANTYNEET };
    enum AvoinLaskuSarake { NUMERO, PVM, ERAPVM, SUMMA, MAKSAMATTA, ASIAKAS };
    enum { TositeRooli = Qt::UserRole + 1 ,
           JsonRooli = Qt::UserRole + 2,
           AvoinnaRooli = Qt::UserRole + 3,
           ViiteRooli = Qt::UserRole + 4,
           AsiakasRooli = Qt::UserRole + 5};

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &item, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public slots:
    void lataaAvoimet();
    void paivita(int valinta = KAIKKI, QDate mista=QDate(), QDate mihin = QDate());

    /**
     * @brief Vähentää laskun avointa määrää ja poistaa jos kokonaan maksettu
     * @param laskuId
     * @param senttia
     */
    void maksa(int indeksi, int senttia);

private:
    QList<AvoinLasku> laskut;

};

#endif // AVOIMETLASKUTMODEL_H
