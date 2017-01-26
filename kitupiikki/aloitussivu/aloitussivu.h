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

#ifndef ALOITUSSIVU_H
#define ALOITUSSIVU_H

#include <QWebEngineView>


#include "db/tilikausi.h"

class Sisalto;


/**
 * @brief Webipohjaiset aloitus- ja ohjesivut
 *
 * QWebEngineView-pohjainen web-sivun näyttäjä, jolla esitetään tilannesidonnaisia opasteita näyttävä aloitussivu
 * sekä ohjelman omat ohjeet.
 *
 */
class AloitusSivu : public QWebEngineView
{
    Q_OBJECT

public:
    AloitusSivu();

public slots:
    /**
     * @brief Näyttää aloitussivun
     * @param Kirjanpito Kirjanpidon tietokantaolio
     */
    void lataaAloitussivu();

    /**
     * @brief Näyttää ohjeiden pääsivun
     */
    void lataaOhje();

    /**
     * @brief Sisältö kutsuu tätä, kun pyydetään selausta selaa:-protokollalla
     * @param tilinumero
     */
    void selaaTilia(int tilinumero);

signals:
    /**
     * @brief Ilmoittaa että käyttäjä on valinnut aloitussivulta toiminnon tai tiedoston avaaamisen
     * @param toiminto toiminto (avaa,uusi) tai avattavan tiedoston koko polku
     */
    void toiminto(const QString toiminto);

    void selaus(int tilinumero, Tilikausi tilikausi);

protected:
    void lisaaTxt(const QString& txt);
    void sivunAloitus();
    void kpAvattu();
    void saldot();
    void alatunniste();

    Sisalto *sisalto;
    Tilikausi tilikausi;
};

#endif // ALOITUSSIVU_H
