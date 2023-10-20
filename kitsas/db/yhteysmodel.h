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
        TOSITE_SELAUS       = 0b00000000000000000001,
        TOSITE_LUONNOS      = 0b00000000000000000010,
        TOSITE_MUOKKAUS     = 0b00000000000000000100,
        TOSITE_KOMMENTTI    = 0b10000000000000000000,
        TOSITE              = 0b10000000000000000111,
        LASKU_SELAUS        = 0b00000000000001000,
        LASKU_LAATIMINEN    = 0b00000000000010000,
        LASKU_LAHETTAMINEN  = 0b00000000000100000,
        TUOTTEET            = 0b01000000000000000,
        RYHMAT              = 0b10000000000000000,
        LASKU               = 0b11000000001110000,
        KIERTO_LISAAMINEN   = 0b0000000000000000001000000,
        KIERTO_TARKASTAMINEN= 0b0000000000000000010000000,
        KIERTO_HYVAKSYMINEN = 0b0000000000000000100000000,
        KIERTO_SELAAMINEN   = 0b0000001000000000000000000,
        MAKSETTAVAKSI       = 0b0100000000000000000000000,
        MAKSETTAVAT         = 0b1000000000000000000000000,
        KIERTO              = 0b1100001000000000111000000,
        ALV_ILMOITUS        = 0b001000000000,
        BUDJETTI            = 0b010000000000,
        TILINPAATOS         = 0b100000000000,
        TYOKALUT            = 0b111000000000,
        RAPORTIT            = 0b100000000000000000,             
        LISAOSA_ASETUKSET   = 0b01100000000000000000000,
        LISAOSA_KAYTTO      = 0b01000000000000000000000,
        PERUSASETUKSET      = 0b10000000000000000000000,
        ASETUKSET           = 0b10000000001000000000000,
        KPHALLINTA            = 0b11100000001000000000000,
        KAYTTOOIKEUDET      = 0b000000000000010000000000000,
        OMISTAJA            = 0b000000000000100000000000000,
        KIRJANPITAJA        = 0b111111111111001111111111111,
        HALLINTA            = 0


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
