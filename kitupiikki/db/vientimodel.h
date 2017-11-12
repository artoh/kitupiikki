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

#ifndef VIENTIMODEL_H
#define VIENTIMODEL_H

#include <QAbstractTableModel>

#include "db/tili.h"
#include "db/kohdennus.h"
#include "db/jsonkentta.h"
#include "verotyyppimodel.h"

class TositeModel;


/**
 * @brief Yhden viennin tiedot. VientiModel käyttää.
 */
struct VientiRivi
{
    int vientiId = 0;
    int riviNro = 0;
    QDate pvm;
    Tili tili;
    QString selite;
    int debetSnt = 0;
    int kreditSnt = 0;
    int alvkoodi = 0;
    int alvprosentti = 0;
    Kohdennus kohdennus;
    QDateTime luotu;
    QDateTime muokattu;
    JsonKentta json;
};

/**
 * @brief Yhden tositteen vientien tiedot
 *
 * Vientejä muokataan tämän modelin kautta, saadaan TositeModelin
 * vientiModel()-funktiolla
 *
 * Mahdollistaa vientien näyttämisen suoraan taulukossa
 *
 */
class VientiModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum VientiSarake
    {
        PVM, TILI, DEBET, KREDIT, KOHDENNUS, ALV, SELITE
    };

    enum
    {
        IdRooli = Qt::UserRole + 1,
        PvmRooli = Qt::UserRole + 2,
        TiliNumeroRooli = Qt::UserRole + 3,
        DebetRooli = Qt::UserRole + 4,
        KreditRooli = Qt::UserRole + 5,
        AlvKoodiRooli = Qt::UserRole + 6,
        AlvProsenttiRooli = Qt::UserRole + 7,
        KohdennusRooli = Qt::UserRole + 8,
        SeliteRooli = Qt::UserRole + 9,
        LuotuRooli = Qt::UserRole +10,
        MuokattuRooli = Qt::UserRole + 11,
        RiviRooli = Qt::UserRole + 12
    };


    VientiModel(TositeModel *tositemodel);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool insertRows(int row, int count, const QModelIndex &);
    bool lisaaRivi();

    /**
     * @brief Poistaa rivin
     * @param rivi Rivin indeksi
     * @return
     */
    void poistaRivi(int rivi);

    bool lisaaVienti(const QDate& pvm, int tilinumero, const QString& selite,
                     int debetSnt, int kreditSnt, int rivinro = 0);

    /**
     * @brief Lisää viennin
     *
     * Lisää tyhjän viennin. Tätä käytetään, kun vienti lisätään ohjelmallisesti ja
     * sitä muokataan setData-rajapintaa käyttäen.
     *
     * @return indeksi uuteen vientiin
     */
    QModelIndex lisaaVienti();

    /**
     * @brief Lisää valmiin vientirivin
     * @param rivi Lisättävä vienti
     *
     * Lisää valmiin viennin. Tätä käytetään, kun KirjausApuriDialogi tallentaa
     * kirjauksen
     */
    QModelIndex lisaaVienti(VientiRivi rivi);


    int debetSumma() const;
    int kreditSumma() const;

    /**
     * @brief Onko vientejä muokattu tallennuksen jälkeen
     * @return
     */
    bool muokattu() const { return muokattu_; }

public slots:
    void tallenna();
    void tyhjaa();
    void lataa();


signals:
    void siirryRuutuun(QModelIndex index);
    void muuttunut();

protected:
    TositeModel *tositeModel_;
    QList<VientiRivi> viennit_;

    VientiRivi uusiRivi();
    int seuraavaRiviNumero();
    bool muokattu_;

    QList<int> poistetutVientiIdt_;
};

#endif // VIENTIMODEL_H
