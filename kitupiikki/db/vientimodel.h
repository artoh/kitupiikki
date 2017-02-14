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

class TositeModel;


namespace AlvKoodi {
    /**
     * @brief Viennin alv-kirjauksen laji
     */
    enum
    {
        EIALV = 0,
        ALVNETTO = 1,
        ALVBRUTTO = 2,
        YHTEISO_TAVARAT = 3,
        YHTEISO_PALVELU = 4,
        KAANNETTY_RAKENNUS = 5,
        ALVKIRJAUS = 99
    };

};

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
        PVM, TILI, DEBET, KREDIT, ALV, KOHDENNUS, SELITE
    };

    enum
    {
        IdRooli, PvmRooli, TiliNumeroRooli, DebetRooli, KreditRooli, AlvKoodiRooli,
        AlvProsenttiRooli, KohdennusRooli, SeliteRooli,
        LuotuRooli, MuokattuRooli, RiviRooli
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

    bool lisaaVienti(const QDate& pvm, int tilinumero, const QString& selite,
                     int debetSnt, int kreditSnt, int rivinro = 0);

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
};

#endif // VIENTIMODEL_H
