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
#ifndef TAVALLINENLASKUDIALOGI_H
#define TAVALLINENLASKUDIALOGI_H

#include "rivillinenlaskudialogi.h"

class EnnakkoHyvitysModel;

class TavallinenLaskuDialogi : public RivillinenLaskuDialogi
{
    Q_OBJECT
public:
    TavallinenLaskuDialogi(Tosite* tosite, QWidget *parent = nullptr);

protected:
    void tositteelle() override;
    QDate toistoPvm() const;

    void toistoTositteelta();
    void paivitaToistojakso();

    void asiakasMuuttui() override;
    void maksuTapaMuuttui() override;
    void hyvitaEnnakko();
    void ennakkoTietoSaapuu(QVariant* data, int eraId, Euro euro);

    void tallennaToisto();
    void lopetaToisto();

    EnnakkoHyvitysModel* ennakkoModel_;

    void merkkaaTallennettu();
    bool onkoTallennettu();
    void paivitaNapit();

    bool tallennettuKaytossa_ = false;
    int tallennettuJakso_ = 0;
    int tallennettuLaskutus_ = 0;
    bool tallennettuEnnen_ = false;
    bool tallennettuHinnastolla_ = false;
    QDate tallennettuPaattyy_;

    bool tarkasta() override;
};

#endif // TAVALLINENLASKUDIALOGI_H
