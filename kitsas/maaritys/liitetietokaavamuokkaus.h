/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef LIITETIETOKAAVAMUOKKAUS_H
#define LIITETIETOKAAVAMUOKKAUS_H

#include "maarityswidget.h"
#include "ui_kaavaeditori.h"

/**
 * @brief The Tilinpäätöksen mallin muokkaaminen
 *
 * Historiallisista syistä luokan nimi viittaa pelkästään liitetietojen kaavaan
 *
 */
class LiitetietokaavaMuokkaus : public MaaritysWidget
{
    Q_OBJECT
public:
    LiitetietokaavaMuokkaus();
    ~LiitetietokaavaMuokkaus();

    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;
    void lataa();

    QString ohjesivu() override { return "maaritykset/tilinpaatos";}

protected slots:
    void ilmoitaOnkoMuokattu();
    void lisaaRaportti();

protected:
    Ui::Kaavaeditori* ui;
};

#endif // LIITETIETOKAAVAMUOKKAUS_H
