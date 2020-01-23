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

/**
  * @dir arkisto
  * @brief Tilikaudet, tilinpäätös ja arkistointi
  */

#ifndef ARKISTO_H
#define ARKISTO_H

#include "kitupiikkisivu.h"
#include "ui_arkisto.h"

#include "db/tilikausi.h"

/**
 * @brief Tilikausisivu
 *
 * Tällä sivulla aloitetaan uusi tilikausi, muokataan viimeisintä,
 * tehdään tilinpäätös tai arkisto tai katsellaan arkistoa taikka
 * tilinpäätöstä
 *
 */
class ArkistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    ArkistoSivu();
    ~ArkistoSivu() override;

    void siirrySivulle() override;

    QString ohjeSivunNimi() override { return "tilikaudet/tilikaudet"; }

public slots:
    static void uusiTilikausi();

    void aineisto();
    void arkisto();
    void vieArkisto();    
    void tilinpaatos();
    void tilinpaatosKasky();
    void nykyinenVaihtuuPaivitaNapit();
    void teeArkisto(Tilikausi kausi);
    void muokkaa();
    void budjetti();    
    void uudellenNumerointi();

private:
    Ui::TilikausiMaaritykset *ui;

    void jatkaVientia(const QString &polku);
    bool teeZip(const QString& polku);
    bool vieHakemistoon(const QString& polku);
};

#endif // ARKISTO_H
