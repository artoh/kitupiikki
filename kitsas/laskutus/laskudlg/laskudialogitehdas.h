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
#ifndef LASKUDIALOGITEHDAS_H
#define LASKUDIALOGITEHDAS_H

#include <QObject>

#include "model/lasku.h"

class KitsasInterface;
class KantaLaskuDialogi;
class Tosite;

class LaskuDialogiTehdas : public QObject
{
    Q_OBJECT

protected:
    LaskuDialogiTehdas(KitsasInterface* kitsas= nullptr, QObject* parent = nullptr);

public:
    static void kaynnista(KitsasInterface* interface, QObject *parent);

    static void naytaLasku(int tositeId);
    static KantaLaskuDialogi* myyntilasku(int asiakasId = 0);
    static KantaLaskuDialogi* ryhmalasku();
    static void hyvityslasku(int hyvitettavaTositeId);
    static void kopioi(int tositeId);

protected:
    void naytaDialogi(Tosite* tosite);
    void tositeLadattu();
    void hyvitettavaLadattu();
    void ladattuKopioitavaksi();

    static Lasku::Rivityyppi oletusRiviTyyppi();

    KitsasInterface* kitsas_ = nullptr;

    static LaskuDialogiTehdas* instanssi__;
    static QDate paivamaara();

signals:

};

#endif // LASKUDIALOGITEHDAS_H
