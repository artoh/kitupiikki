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
#ifndef OTERIVI_H
#define OTERIVI_H

#include "model/euro.h"
#include "laskutus/iban.h"
#include <QDate>
#include "tilioteotsake.h"

namespace Tuonti {


class OteRivi
{
public:
    OteRivi();

    void kasittele(const QString& teksti, TilioteOtsake::Tyyppi tyyppi, int rivi, const QDate& loppupvm);

    void setEuro(const QString maara);
    void setArkistotunnus(QString arkistotunnus, int rivi);
    void setSaajaMaksaja(const QString& nimi, int rivi);
    void setSelite(const QString& selite, int rivi);

    void setKTO(int kto) { kto_ = kto;}
    void setViite(QString viite);
    void setIban(const Iban& iban);
    void setPvm(const QDate& pvm) { pvm_ = pvm;}
    void setPvm(const QString& str,
                const QDate &loppupvm);

    Euro euro() const { return euro_;}
    QString arkistotunnus() const { return arkistotunnus_;}
    bool valmis() const;

    void kasitteleTuntematon(const QString& teksti, int rivi, const QDate &loppupvm);

    void lisaaYleinen(const QString& teksti, int rivi);

    static int ktoKoodi(const QString& teksti);
    static QDate strPvm(const QString& str,
                       const QDate &loppupvm);

    void tyhjenna();
    QVariantMap map(const QDate& kirjauspvm) const;

protected:
    void lisaaOstoPvm(const QString& teksti);

    enum Tila { NORMAALI, MAKSAJANVIITE, VIESTI, OSTOPVM, OHITALOPPUUN};

    Tila tila = NORMAALI;
    Euro euro_;
    QString arkistotunnus_;
    QString saajamaksaja_;
    int kto_ = 0;
    QString viite_;
    Iban iban_;
    QDate pvm_;
    QString viesti_;
    QDate ostopvm_;
    int arkistoTunnusRivi_ = -1;

    int saajaMaksajaRivi_ = -1;

    static std::vector<QString> ohitettavat__;    
    static QRegularExpression ibanRe__;
    static QRegularExpression nroRe__;
    static QRegularExpression emailRe__;
    static QRegularExpression tekstiRe__;
};


}
#endif // OTERIVI_H
