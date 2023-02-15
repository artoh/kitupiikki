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
#ifndef PDFTILIOTETUONTI_H
#define PDFTILIOTETUONTI_H

#include <QVariant>
#include "tilioteotsake.h"
#include "oterivi.h"

#include <QRegularExpression>
#include <QDate>
#include "laskutus/iban.h"



namespace Tuonti {

class PdfTiedosto;
class PdfSivu;
class PdfRivi;

class PdfTilioteTuonti
{
public:
    PdfTilioteTuonti();    
    QVariantMap tuo(PdfTiedosto* tiedosto);

    QString bic() const { return iban_.bic();}

protected:
    enum { TILIOTETYYPPI = 400 };
    QVariantMap map() const;

    void lueRivi(PdfRivi* rivi);

    void lueAlkuRivi(PdfRivi* rivi);
    void lueOtsakeRivi(PdfRivi *rivi);
    void lueTaulukkoRivi(PdfRivi *rivi);
    void lueToisenAlkua(PdfRivi* rivi);

    void kasitteleTaulukkoRivi(PdfRivi *rivi);

    void nykyinenValmis();

    enum Status { ALKU, OTSAKE, TAULU, LOPPU, TOINENSIVU };

    Status tila_ = ALKU;

    QDate alkupvm_;
    QDate loppupvm_;
    Iban iban_;
    QString kausiTeksti_;

    QVariantList tapahtumat_;
    OteRivi nykyinen_;
    int rivilla_ = 0;

    double edellinenAlalaita_;

    TilioteOtsake otsake_;

    QDate kirjausPvm_;

    static QRegularExpression kauttaRe__;
    static QRegularExpression valiReViivalla__;
    static QRegularExpression rahaRe__;
    static QRegularExpression omaIbanRe__;
    static QRegularExpression numeroRe__;
    static QRegularExpression pieniRe__;
    static QRegularExpression aakkosRe__;

    static std::vector<QString> jatkuuTekstit__;
};

};

#endif // PDFTILIOTETUONTI_H
