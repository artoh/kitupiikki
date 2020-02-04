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
#ifndef YHTEYSMODEL_H
#define YHTEYSMODEL_H

#include <QAbstractListModel>
#include "kpkysely.h"

/**
 * @brief Tietokantayhteyksien kantaluokka
 *
 * YhteysModel sisältää listan yhteyden takana aktiivisena olevista kirjanpidoista.
 * Lisäksi sen kautta luodaan kyselyt
 *
 */
class YhteysModel : public QAbstractListModel
{
public:
    enum Oikeus {
        TOSITE_SELAUS       = 0b1,
        TOSITE_LUONNOS      = 0b10,
        TOSITE_MUOKKAUS     = 0b100,
        LASKU_SELAUS        = 0b1000,
        LASKU_LAATIMINEN    = 0b10000,
        LASKU_LAHETTAMINEN  = 0b100000,
        KIERTO_LISAAMINEN   = 0b1000000,
        KIERTO_TARKASTAMINEN= 0b10000000,
        KIERTO_HYVAKSYMINEN = 0b100000000,
        ALV_ILMOITUS        = 0b1000000000,
        BUDJETTI            = 0b10000000000,
        TILINPAATOS         = 0b100000000000,
        ASETUKSET           = 0b1000000000000,
        KAYTTOOIKEUDET      = 0b10000000000000,
        OMISTAJA            = 0b100000000000000,
        TUOTTEET            = 0b1000000000000000,
        RYHMAT              = 0b10000000000000000,
        RAPORTIT            = 0b100000000000000000,
        KIERTO_SELAAMINEN   = 0b1000000000000000000
    };


    YhteysModel(QObject *parent = nullptr);

    virtual KpKysely* kysely(const QString& polku = QString(),
                             KpKysely::Metodi metodi = KpKysely::GET) = 0;

    virtual void sulje() = 0;
    void alusta();
    void lataaInit(QVariant* reply);
    virtual qlonglong oikeudet() const = 0;
    bool onkoOikeutta(qlonglong oikeus);

private slots:
    void initSaapuu(QVariant* reply);


};

#endif // YHTEYSMODEL_H
