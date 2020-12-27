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
#ifndef RAPORTINMUOKKAUS_H
#define RAPORTINMUOKKAUS_H

#include "../maarityswidget.h"
#include "db/kielikentta.h"

namespace Ui {
    class RaportinMuokkaus;
}

class RaporttiMuokkausModel;


class RaportinMuokkaus : public MaaritysWidget
{
    Q_OBJECT
public:
    RaportinMuokkaus(QWidget *parent = nullptr);

    bool nollaa() override;
    bool onkoMuokattu() override;
    bool tallenna() override;
    bool naytetaankoVienti() override { return true; }


protected:
    void lataa(const QString& raportti);
    void muokkaaNimikkeet();
    void muokkaa();
    void paivitaNapit(const QModelIndex& index);
    void ilmoitaMuokattu();

    void kopioiRaportti();

    void lisaaEnnen();
    void lisaaJalkeen();
    void poista();

    QString data() const;

    Ui::RaportinMuokkaus* ui;
    RaporttiMuokkausModel* model_;
    KieliKentta nimi_;
    KieliKentta muoto_;

};

#endif // RAPORTINMUOKKAUS_H
