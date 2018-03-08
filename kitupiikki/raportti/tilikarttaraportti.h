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

#ifndef TILIKARTTARAPORTTI_H
#define TILIKARTTARAPORTTI_H

#include "ui_tilikarttaraportti.h"

#include "raportti.h"
#include "raportinkirjoittaja.h"

#include "db/kirjanpito.h"

/**
 * @brief Tilikartan tulostava raportti
 */
class TilikarttaRaportti : public Raportti
{
    Q_OBJECT
public:
    enum KarttaValinta
    {
        KAIKKI_TILIT,
        KAYTOSSA_TILIT,
        KIRJATUT_TILIT,
        SUOSIKKI_TILIT
    };


    TilikarttaRaportti();
    ~TilikarttaRaportti();


    RaportinKirjoittaja raportti( bool csvmuoto );

    static RaportinKirjoittaja kirjoitaRaportti(KarttaValinta valinta, Tilikausi tilikaudelta,
                             bool tulostatyypi, QDate saldopvm, bool kirjausohjeet, bool csv = false);

protected slots:
    /**
     * @brief Kun tilikausi vaihtuu, päivitetään saldopäivä sille
     */
    void paivitaPaiva();
protected:
    Ui::TilikarttaRaportti *ui;
};

#endif // TILIKARTTARAPORTTI_H
