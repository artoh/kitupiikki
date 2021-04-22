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
#ifndef RIVILLINENLASKUDIALOGI_H
#define RIVILLINENLASKUDIALOGI_H

#include "yksittainenlaskudialogi.h"
#include "../laskualvcombo.h"
#include "../tositerivialv.h"
#include "model/lasku.h"

class RivillinenLaskuDialogi : public YksittainenLaskuDialogi
{
    Q_OBJECT
public:
    RivillinenLaskuDialogi(Tosite* tosite, QWidget* parent);
    LaskuAlvCombo::AsiakasVeroLaji asiakasverolaji() const;

    Lasku::Rivityyppi rivityyppi() const;

protected:
    void tuotteidenKonteksiValikko(QPoint pos);
    void uusiRivi();
    void rivinLisaTiedot();
    void paivitaRiviNapit();
    void tositteelle() override;

    void valmisteleTallennus() override;

    TositeriviAlv* alvTaulu() { return &alv_;}

    bool tarkasta() override;
    virtual void lisaaTuote(const QModelIndex& index);

    void vaihdaRivilajia(const QString& asiakkaanAlvTunnus );
    void riviTyyppiVaihtui();

private:
    void alustaRiviTab();
    void alustaRiviTyypit();
    void paivitaSumma();    

private:
    TositeriviAlv alv_;

};

#endif // RIVILLINENLASKUDIALOGI_H
