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
#include <QList>

#include "db/tili.h"
#include "db/kohdennus.h"
#include "db/jsonkentta.h"
#include "verotyyppimodel.h"
#include "db/eranvalintamodel.h"

class TositeModel;


/**
 * @brief Yhden viennin tiedot. VientiModel käyttää.
 */
struct VientiRivi
{   
    int vientiId = 0;
    QDate pvm;
    Tili tili;
    QString selite;
    qlonglong debetSnt = 0;
    qlonglong kreditSnt = 0;
    int alvkoodi = 0;
    int alvprosentti = 0;
    Kohdennus kohdennus;
    int eraId = 0;
    QDateTime luotu;
    QDateTime muokattu;
    JsonKentta json;
    QString viite;
    QString ibanTili;
    QDate erapvm;
    QString asiakas;
    QString arkistotunnus;
    QList<Kohdennus> tagit;
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
        RiviRooli = Qt::UserRole + 12,
        EraIdRooli = Qt::UserRole + 13,
        PoistoKkRooli = Qt::UserRole + 14,
        TaseErittelyssaRooli = Qt::UserRole + 15,
        ViiteRooli = Qt::UserRole + 16,
        SaajanTiliRooli = Qt::UserRole + 17,
        SaajanNimiRooli = Qt::UserRole + 18,
        EraPvmRooli = Qt::UserRole + 19,
        ArkistoTunnusRooli = Qt::UserRole + 20,
        TagiNimilistaRooli = Qt::UserRole + 21,
        TagiIdListaRooli = Qt::UserRole + 22,
        AsiakasRooli = Qt::UserRole + 23
    };


    VientiModel(TositeModel *tositemodel);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool insertRows(int row, int count, const QModelIndex &);

    /**
     * @brief Poistaa rivin
     * @param rivi Rivin indeksi
     * @return
     */
    void poistaRivi(int rivi);

    /**
     * @brief Lisää viennin
     *
     * Lisää tyhjän viennin. Tätä käytetään, kun vienti lisätään ohjelmallisesti ja
     * sitä muokataan setData-rajapintaa käyttäen.
     *
     * @return indeksi uuteen vientiin
     */
    QModelIndex lisaaVienti(int indeksi = -1);

    /**
     * @brief Lisää valmiin vientirivin
     * @param rivi Lisättävä vienti
     *
     * Lisää valmiin viennin. Tätä käytetään, kun KirjausApuriDialogi tallentaa
     * kirjauksen
     */
    QModelIndex lisaaVienti(VientiRivi rivi, int indeksi = -1);


    qlonglong debetSumma() const;
    qlonglong kreditSumma() const;

    /**
     * @brief Onko vientejä muokattu tallennuksen jälkeen
     * @return
     */
    bool muokattu() const { return muokattu_; }

public slots:
    /**
     * @brief Tallentaa viennit
     * @return tosi, jos onnistui
     */
    bool tallenna(bool tallennatyhjat = false);
    void tyhjaa();
    void lataa();


signals:
    void siirryRuutuun(QModelIndex index);
    void muuttunut();

protected:
    TositeModel *tositeModel_;
    QList<VientiRivi> viennit_;

    VientiRivi uusiRivi();
    bool muokattu_;

    QList<int> poistetutVientiIdt_;
};

#endif // VIENTIMODEL_H
