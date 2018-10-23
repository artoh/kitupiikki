/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#ifndef RAPORTTIMUOKKAUS_H
#define RAPORTTIMUOKKAUS_H

#include "maarityswidget.h"
#include "ui_raportinmuokkaus.h"

/**
 * @brief Sivu, jolla muokataan raportteja
 */
class RaporttiMuokkaus : public MaaritysWidget
{
    Q_OBJECT

public:
    RaporttiMuokkaus(QWidget *parent=0);
    ~RaporttiMuokkaus();

    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;

    QString ohjesivu() override { return "maaritykset/raportit";}

protected slots:
    void avaaRaportti(const QString& raportti);
    void merkkaaMuokattu();
    void uusi();
    void kopio();
    void nimea();
    void poista();

protected:
    /**
     * @brief Kysyy, tallennetaanko muokattu
     *
     * Tallentaa, jos halutaan niin
     *
     * @return Voiko poistua
     */
    bool kysyTallennus();

    bool aloitaUusi();

    /**
     * @brief Arkiston liittäminen arkistoon
     * @param laitetaanko Laitetaanko
     * @param vertailu Onko budjettivertailu
     */
    void raportinArkistointi(bool laitetaanko, bool vertailu);

    Ui::RaporttiMuokkain *ui;
    QString nimi;
    bool muokattu;
};

#endif // RAPORTTIMUOKKAUS_H
