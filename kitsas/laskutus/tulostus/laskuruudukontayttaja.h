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
#ifndef LASKURUUDUKONTAYTTAJA_H
#define LASKURUUDUKONTAYTTAJA_H

#include "model/tosite.h"
#include "tulostusruudukko.h"
#include "model/tositerivi.h"

#include <QList>

class KitsasInterface;

class LaskuRuudukonTayttaja
{
public:
    LaskuRuudukonTayttaja(KitsasInterface* kitsas);

    TulostusRuudukko tayta(Tosite &tosite);

private:
    void tutkiSarakkeet(Tosite& tosite);

    void kirjoitaSarakkeet();
    void lisaaSarake(const QString& koodi, Qt::AlignmentFlag tasaus = Qt::AlignLeft);

    void taytaSarakkeet(Tosite& tosite);
    QString nimikesarake(const TositeRivi& rivi);
private:

    KitsasInterface* kitsas_;

    bool alvSarake_ = false;
    bool aleSarake_ = false;

    TulostusRuudukko ruudukko_;
    QString kieli_;
};



#endif // LASKURUUDUKONTAYTTAJA_H
