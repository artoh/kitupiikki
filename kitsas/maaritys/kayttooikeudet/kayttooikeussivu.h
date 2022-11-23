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
#ifndef KAYTTOOIKEUSSIVU_H
#define KAYTTOOIKEUSSIVU_H

#include "../maarityswidget.h"
#include "kayttooikeusmodel.h"
#include <QSet>

namespace Ui {
    class KayttoOikeudet;
    class OikeusWidget;
}



class KayttoOikeusSivu : public MaaritysWidget
{
    Q_OBJECT
public:
    KayttoOikeusSivu();
    ~KayttoOikeusSivu() override;

    bool nollaa() override;
    bool naytetaankoTallennus() override { return false;}

private:
    void naytaValittu(const QModelIndex &index);
    void malliNollattu();
    void tarkastaNimi();
    void lisaaKayttaja();
    void tallennaOikeudet();
    void tallennettu();
    void tarkastaMuokattu();
    void kaikkiOikeudet();
    void poistaOikeudet();
    void kutsu();
    void uusiKutsu();

private:
    Ui::KayttoOikeudet* ui;
    Ui::OikeusWidget *oikeusUi;
    KayttooikeusModel* model;

    QString nykyisenEmail_;
    QString haettuNimi_;    
    bool omistaja_;
};

#endif // KAYTTOOIKEUSSIVU_H
