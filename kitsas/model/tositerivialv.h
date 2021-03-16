/*
   Copyright (C) 2019 Arto Hyvättinen

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
#ifndef TOSITERIVIALV_H
#define TOSITERIVIALV_H

#include <QMap>
#include "euro.h"

class TositeRivit;


class TositeriviAlv
{
public:
    TositeriviAlv(TositeRivit* rivit);

    Euro netto(int indeksi = -1) const;
    Euro vero(int indeksi = -1) const;
    Euro brutto(int indeksi = -1) const;
    int alvkoodi(int indeksi) const;
    double veroprosentti(int indeksi) const;

    bool bruttoPeruste() { return  bruttoperuste_;}
    void asetaBruttoPeruste(bool onko) { bruttoperuste_ = onko; paivita();}

    void paivita();
    QList<int> indeksitKaytossa() const;

protected:
    class AlvTieto {
    public:
        AlvTieto();
        AlvTieto(int verokoodi, double veroprosentti);

        int verokoodi() const { return verokoodi_;}
        double veroProsentti() const { return veroProsentinSadasosat_ / 100.0;}

        Euro netto() const { return netto_;}
        Euro vero() const { return vero_;}
        Euro brutto() const { return brutto_;}

        void lisaaNettoon(const Euro& euro) { netto_ += euro;}
        void lisaaVeroon(const Euro& euro) { vero_ += euro;}
        void lisaaBruttoon(const Euro& euro) { brutto_ += euro;}

        void paivita(bool bruttoperuste);
        void clear();

    private:
        int verokoodi_;
        int veroProsentinSadasosat_;

        Euro netto_;
        Euro vero_;
        Euro brutto_;
    };

    Euro netto_;
    Euro vero_;
    Euro brutto_;

    QList<AlvTieto> tiedot_;
    bool bruttoperuste_ = true;
    TositeRivit* rivit_;

};

#endif // TOSITERIVIALV_H
