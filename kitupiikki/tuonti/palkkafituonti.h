/*
   Copyright (C) 2018 Arto Hyvättinen

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

#ifndef PALKKAFITUONTI_H
#define PALKKAFITUONTI_H

#include <QDate>
#include <QMap>
#include "tuonti.h"


/**
 * @brief Kirjanpitoaineiston tuonti palkka.fi-palvelusta
 *
 * Tuo csv-muotoisen kirjanpitotositteen palkka.fi-palvelusta
 *
 * Tilien muunnos tehdään tilikartan [PalkkaFiTuonti] määritteellä, jossa jokaisella rivillä on välilyönnillä
 * erotettuna ensin palkka.fi-tilinumero ja sitten Kitupiikissä käytettävä tilinumero
 *
 */
class PalkkaFiTuonti : public Tuonti
{
public:
    PalkkaFiTuonti(KirjausWg *wg);

    bool tuo(const QByteArray &data) override;

protected:
    void tuoRivi(const QString& rivi);

    QDate pvm_;
    QString otsikko_;
    QMap<int,int> muunto_;
};

#endif // PALKKAFITUONTI_H
