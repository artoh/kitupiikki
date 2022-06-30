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
#ifndef FINVOICETOIMITTAJA_H
#define FINVOICETOIMITTAJA_H

#include "abstraktitoimittaja.h"

class FinvoiceToimittaja : public AbstraktiToimittaja
{
    Q_OBJECT
public:
    FinvoiceToimittaja(QObject* parent);

protected:
    virtual void toimita() override;

    void alustaInit();
    void paikallinenVerkkolasku();
    void laskuSaapuu(QVariant* data);

    QVariantMap finvoiceJson();


    void aloitaMaventa();
    void liitaLaskunKuva();
    void liiteLiitetty(QVariant* data);
    void lahetaMaventa();
    void maventaToimitettu(QVariant* data);
    void maventaVirhe(int koodi, const QString& selitys);



    QVariantMap init_;    

    Tosite* toimitettavaTosite_;
    QVariantList liitteet_;
    int liiteIndeksi_ = -1;

};

#endif // FINVOICETOIMITTAJA_H
