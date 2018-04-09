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

#include "db/jsonkentta.h"

/**
 * @brief Laskunmaksudialogissa näytettävä avoin lasku
 */
struct AvoinLasku
{
    AvoinLasku() {}

    QString viite;
    QDate pvm;
    QDate erapvm;
    int summaSnt = 0;
    int avoinSnt = 0;
    QString asiakas;
    int tosite = -1;
    int kirjausperuste = 0;
    int eraId = 0;
    int tiliid = 0;
    JsonKentta json;
    int kohdennusId;
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
           AvoinnaRooli = Qt::UserRole + 3,
           ViiteRooli = Qt::UserRole + 4,
           AsiakasRooli = Qt::UserRole + 5,
           LiiteRooli = Qt::UserRole + 6,
           HyvitysLaskuRooli = Qt::UserRole + 7,
           KirjausPerusteRooli = Qt::UserRole + 8,
           KohdennusIdRooli = Qt::UserRole + 9,
           EraIdRooli = Qt::UserRole + 10,
           TiliIdRooli = Qt::UserRole + 11};

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &item, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    AvoinLasku laskunTiedot(int indeksi) const;

public slots:
    void lataaAvoimet();
    void paivita(int valinta = KAIKKI, QDate mista=QDate(), QDate mihin = QDate());

    /**
     * @brief Vähentää laskun avointa määrää ja poistaa jos kokonaan maksettu
     * @param laskuId
     * @param senttia
     */
    void maksa(int indeksi, int senttia);

public:
    /**
     * @brief Palauttaa BIC-koodin suomalaisella IBAN-tilinumerolla
     * @param iban IBAN-tilinumero
     * @return
     */
    static QString bicIbanilla(const QString& iban);

protected:
    QList<AvoinLasku> laskut;

};

#endif // AVOIMETLASKUTMODEL_H
